import {type NodePath, types as t} from '@babel/core'
import {containsStateGetterCall, generateShortId} from '../utils'
import {handleJsxElement} from "./JSXTransform";
import type {BabelFunction, FunctionScope} from "./types";


export function processFunc(path: BabelFunction) {
    if (!containsJSX(path)) {
        //If no jsx let's skip it
        path.skip()
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
        VariableDeclarator(path) {
            const init = path.node.init;
            const id = path.node.id;

            if (!t.isCallExpression(init)) return;

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

            if (t.isIdentifier(id)) {
                // Single variable case
                funcState.variables.add(id.name);
            } else if (t.isObjectPattern(id)) {
                // Destructuring case (object)
                id.properties.forEach(prop => {
                    if (t.isObjectProperty(prop) && t.isIdentifier(prop.key)) {
                        funcState.variables.add(prop.key.name);
                    } else if (t.isRestElement(prop) && t.isIdentifier(prop.argument)) {
                        funcState.variables.add(prop.argument.name);
                    }
                });
            } else if (t.isArrayPattern(id)) {
                // Destructuring case (array)
                id.elements.forEach(el => {
                    if (t.isIdentifier(el)) {
                        funcState.variables.add(el.name);
                    } else if (t.isRestElement(el) && t.isIdentifier(el.argument)) {
                        funcState.variables.add(el.argument.name);
                    }
                });
            }
        },
        JSXElement(path) {
            const result = handleJsxElement(path, funcState);
            // This case shouldn't happen
            if (result.statements.length !== 0) {
                throw new Error("Statement return with handling  " + JSON.stringify(path.node.openingElement.name))
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
        }
    })
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
