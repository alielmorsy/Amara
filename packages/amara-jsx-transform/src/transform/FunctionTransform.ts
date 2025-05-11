import {type NodePath, types as t} from '@babel/core'
import {containsStateGetterCall, generateShortId} from '../utils'
import {handleJsxElement} from "./JSXTransform";
import type {BabelFunction, FunctionScope} from "./types";


export function processFunc(path: BabelFunction) {
    if (!containsJSX(path)) {
        return;
    }
    if (isInsideAnotherFunction(path)) return;

    const body = path.node.body;

    //In the actual specification components has no shortIDs. But, I believe
    const componentBeginExpression = t.callExpression(t.identifier("beginComponentInit"), [t.stringLiteral(generateShortId())]);
    const startCall = t.expressionStatement(componentBeginExpression);
    if (t.isBlockStatement(body)) {
        // For normal functions with `{}` bodies we add startCall at the top
        body.body.unshift(startCall);
    } else {
        // For arrow functions with implicit return (e.g., `() => <div />`)
        path.node.body = t.blockStatement([startCall, t.returnStatement(body)]);
    }
    const funcState: FunctionScope = {
        variables: new Set<string>(),
        specialFlags: {},
        innerFunctions: []
    }

    path.node.params.forEach(param => {
        if (t.isIdentifier(param)) {
            // Simple prop like: (props) => ...
            funcState.variables.add(param.name);
        } else if (t.isObjectPattern(param)) {
            // Destructured props like: ({ name, age }) => ...
            param.properties.forEach(prop => {
                if (t.isObjectProperty(prop) && t.isIdentifier(prop.key)) {
                    funcState.variables.add(prop.key.name);
                } else if (t.isRestElement(prop) && t.isIdentifier(prop.argument)) {
                    funcState.variables.add(prop.argument.name);
                }
            });
        } else if (t.isAssignmentPattern(param) && t.isIdentifier(param.left)) {
            // Default props like: (name = 'default') => ...
            funcState.variables.add(param.left.name);
        }
    });
    path.traverse({
        FunctionDeclaration: {
            enter(path, state) {
                // Push new scope
                funcState.innerFunctions.push({scopedVariables: new Set<string>()});
            },
            exit(path, state) {
                for (const pathElement in funcState.innerFunctions[funcState.innerFunctions.length - 1].scopedVariables) {
                    funcState.variables.delete(pathElement)
                }
                funcState.innerFunctions.pop();
            }
        },

        FunctionExpression: {
            enter(path, state) {
                funcState.innerFunctions.push({scopedVariables: new Set<string>()});
            },
            exit(path, state) {
                for (const pathElement in funcState.innerFunctions[funcState.innerFunctions.length - 1].scopedVariables) {
                    funcState.variables.delete(pathElement)
                }
                funcState.innerFunctions.pop();
            }
        },

        ArrowFunctionExpression: {
            enter(path, state) {
                funcState.innerFunctions.push({scopedVariables: new Set<string>()});
            },
            exit(path, state) {
                for (const pathElement in funcState.innerFunctions[funcState.innerFunctions.length - 1].scopedVariables) {
                    funcState.variables.delete(pathElement)
                }
                funcState.innerFunctions.pop();
            }
        },

        VariableDeclarator(path) {
            const init = path.node.init;
            const id = path.node.id;
            const parent = path.findParent((p) => {
                return t.isCallExpression(p.node) && t.isIdentifier(p.node.callee, {name: "effect"})
            });
            if (parent) {
                //Ignore effects
                return;
            }
            if (!t.isCallExpression(init)) {
                if (t.isIdentifier(init)) {
                    //The last check is a workaround for dumb users who may redefine variables
                    if (funcState.variables.has(init.name) && t.isIdentifier(id) && !funcState.variables.has(id.name)) {
                        funcState.variables.add(id.name)
                        if (funcState.innerFunctions.length !== 0) {
                            funcState.innerFunctions[funcState.innerFunctions.length - 1].scopedVariables.add(id.name);
                        }
                    }
                }

                return;
            }

            funcState.variables = funcState.variables ?? new Set();

            if (t.isIdentifier(init.callee, {name: "useState"})) {
                if (t.isArrayPattern(id)) {
                    const [getter] = id.elements;
                    if (t.isIdentifier(getter)) {
                        funcState.variables.add(getter.name);
                    }
                }
                return;
            }

            // if (t.isIdentifier(id)) {
            //     // Single variable case
            //     funcState.variables.add(id.name);
            // } else if (t.isObjectPattern(id)) {
            //     // Destructuring case (object)
            //     id.properties.forEach(prop => {
            //         if (t.isObjectProperty(prop) && t.isIdentifier(prop.key)) {
            //             funcState.variables.add(prop.key.name);
            //         } else if (t.isRestElement(prop) && t.isIdentifier(prop.argument)) {
            //             funcState.variables.add(prop.argument.name);
            //         }
            //     });
            // } else if (t.isArrayPattern(id)) {
            //     // Destructuring case (array)
            //     id.elements.forEach(el => {
            //         if (t.isIdentifier(el)) {
            //             funcState.variables.add(el.name);
            //         } else if (t.isRestElement(el) && t.isIdentifier(el.argument)) {
            //             funcState.variables.add(el.argument.name);
            //         }
            //     });
            // }
        },
        JSXElement(path) {
            const result = handleJsxElement(path, funcState);
            // This case shouldn't happen
            if (result.statements.length !== 0) {
                //      throw new Error("Statement return with handling  " + JSON.stringify(path.node.openingElement.name))
            }
            path.skip()
        },
        IfStatement(path) {
            const {consequent, alternate, test} = path.node;
            let isConsequentJSXReturn = false;
            let isAlternateJSXReturn = false;
            if (t.isBlockStatement(consequent)) {
                const returnStmt = consequent.body.find(n => t.isReturnStatement(n));
                isConsequentJSXReturn = isJSXReturn(returnStmt);
            } else {
                isConsequentJSXReturn = isJSXReturn(consequent);
            }

            if (t.isBlockStatement(alternate)) {
                const returnStmt = alternate.body.find(n => t.isReturnStatement(n));
                isAlternateJSXReturn = isJSXReturn(returnStmt);
            } else {
                isAlternateJSXReturn = isJSXReturn(alternate);
            }
            const usedExpression = containsStateGetterCall(test, funcState)
            if (usedExpression && (isConsequentJSXReturn || isAlternateJSXReturn)) {
                const updater = t.expressionStatement(t.callExpression(t.identifier("refresh"), []))
                const arrowFunction = t.arrowFunctionExpression([], t.blockStatement([updater]));
                const depsExpression = t.arrayExpression([t.identifier(usedExpression)]);
                const effect = t.callExpression(t.identifier("effect"), [arrowFunction, depsExpression]);

                path.insertBefore([t.expressionStatement(effect)])
            }
        },
        // NEW: Add visitors for JSX attributes and expressions to wrap reactive variables
        CallExpression(path) {
            handleReactiveVariableInCallExpression(path, funcState);
        }, ObjectExpression(path) {
            handleReactiveVariablesInObject(path, funcState);
        },
        // Add visitor for array expressions to handle reactive variables in arrays
        ArrayExpression(path) {
            handleReactiveVariablesInArray(path, funcState);
        }

    });
}

function containsJSX(path: NodePath) {
    let foundJSX = false;
    path.traverse({
        JSXElement(path) {
            foundJSX = true;
            path.stop();
        }
    });
    return foundJSX;
}

function isInsideAnotherFunction(path: NodePath) {
    let parent = path.findParent((p) => p.isFunction());
    return parent && parent !== path.parentPath;
}

function isJSXReturn(node?: t.Statement | null) {
    if (!t.isReturnStatement(node)) return false;
    return t.isJSXElement(node.argument) || t.isJSXFragment(node.argument);
}


/**
 * Handle reactive variables in call expressions (like function arguments)
 */
function handleReactiveVariableInCallExpression(path: NodePath<t.CallExpression>, funcState: FunctionScope) {
    const {callee, arguments: args} = path.node;

    // Check if this is a call to addStaticChild, insertChild, or similar
    if (t.isIdentifier(callee) && callee.name === "toRaw") {

        return
    }
    // For each argument, check if it's a reactive variable that needs unwrapping
    args.forEach((arg, index) => {
        if (t.isIdentifier(arg) &&
            funcState.variables.has(arg.name) &&
            !isInsideReactiveContext(path)) {

            // Replace with toRaw(varName)
            args[index] = t.callExpression(
                t.identifier("toRaw"),
                [arg]
            );
        } else if (t.isObjectExpression(arg)) {
            // Handle object properties that might contain reactive variables
            arg.properties.forEach(prop => {
                if (t.isObjectProperty(prop) &&
                    t.isIdentifier(prop.value) &&
                    funcState.variables.has(prop.value.name) &&
                    !isInsideReactiveContext(path)) {

                    prop.value = t.callExpression(
                        t.identifier("toRaw"),
                        [prop.value]
                    );
                }
            });
        }
    });
}

/**
 * Check if the current path is inside a reactive context like effect dependencies
 * where we should NOT unwrap reactive variables
 */
function isInsideReactiveContext(path: NodePath) {
    let current: NodePath | null = path;

    while (current) {
        // Inside effect dependency array
        if (t.isArrayExpression(current.node) &&
            current.parent &&
            t.isCallExpression(current.parent) &&
            t.isIdentifier(current.parent.callee, {name: "effect"})) {
            return true;
        }

        // Inside setState, setters, or other reactive operations
        if (t.isCallExpression(current.node) &&
            t.isIdentifier(current.node.callee) &&
            (current.node.callee.name === "listConcile" ||
                current.node.callee.name === "setChild")) {
            return true;
        }

        current = current.parentPath;
    }

    return false;
}

function handleReactiveVariablesInObject(path: NodePath<t.ObjectExpression>, funcState: FunctionScope) {
    // Skip if we're inside a reactive context
    if (isInsideReactiveContext(path)) {
        return;
    }

    const {properties} = path.node;

    // Process each property
    properties.forEach((prop, index) => {
        if (t.isObjectProperty(prop)) {
            if (isReactiveVariable(prop.value as t.Expression, funcState)) {
                // Replace with toRaw(varName)
                prop.value = wrapWithToRaw(prop.value as t.Expression);
            }
        } else if (t.isSpreadElement(prop) && isReactiveVariable(prop.argument, funcState)) {
            // Handle spread elements like { ...reactiveObj }
            prop.argument = wrapWithToRaw(prop.argument);
        }
    });
}

function isReactiveVariable(node: t.Expression, funcState: FunctionScope) {
    return t.isIdentifier(node) &&
        funcState.variables.has(node.name);
}

function wrapWithToRaw(node: t.Expression) {
    return t.callExpression(t.identifier("toRaw"), [node]);
}

/**
 * Handle reactive variables in array expressions
 */
function handleReactiveVariablesInArray(path: NodePath<t.ArrayExpression>, funcState: FunctionScope) {
    // Skip if we're inside a reactive context
    if (isInsideReactiveContext(path)) {
        return;
    }

    const {elements} = path.node;

    // Process each element
    elements.forEach((element, index) => {
        if (!element) return; // Skip null elements

        if (isReactiveVariable(element as t.Expression, funcState)) {
            // Replace with toRaw(varName)
            elements[index] = wrapWithToRaw(element as t.Expression);
        }

        // Handle nested objects
        if (t.isObjectExpression(element)) {
            const childPath = path.get("elements")[index] as NodePath<t.ObjectExpression>;
            handleReactiveVariablesInObject(childPath, funcState);
        }

        // Handle nested arrays
        if (t.isArrayExpression(element)) {
            const childPath = path.get("elements")[index] as NodePath<t.ArrayExpression>;
            handleReactiveVariablesInArray(childPath, funcState);
        }

        // Handle spread elements like [...reactiveArray]
        if (t.isSpreadElement(element) && isReactiveVariable(element.argument, funcState)) {
            element.argument = wrapWithToRaw(element.argument);
        }
    });
}
