import {type NodePath, traverse, types as t} from '@babel/core'
import {
    containsStateGetterCall,

    createObjectProperty, ensureReturnInMapCallback,
    extractMapInfo,
    generateShortId, getJsxElementName,
    getPropertyKey,
    INTERNAL_COMPONENTS,
    isMapExpression, MapInfo
} from '../utils'
import {FunctionScope, LiteralType} from "./types";
import {Identifier} from "@babel/types";

export function createMapHandler(mapInfo: MapInfo, parentPath: NodePath, funcState: FunctionScope, parentElement: t.Identifier, path: NodePath): t.ExpressionStatement {
    const {array, callback, itemParam, indexParam, stateDependency, stateVars} = mapInfo;
    funcState.specialFlags.insideMapHandler = true;
    // If state dependent, use listConcile with effect
    if (stateDependency || stateVars.length > 0) {
        // Create a modified callback that returns an appropriate object structure
        let callbackFunction: t.ArrowFunctionExpression;

        // Prepare parameters for the new callback
        const valueParam = itemParam[0] || path.scope.generateUidIdentifier("item");
        const idxParam = path.scope.generateUidIdentifier(indexParam || "index");
        parentPath.traverse({
            JSXElement(path) {
                const result = handleJsxElement(path, funcState, undefined, true, false)
                path.replaceWith(result.expression!)
            }
        })
        callbackFunction = t.arrowFunctionExpression(
            [valueParam, idxParam],
            callback.body
        );

        // Create listConcile call
        const concileCall = t.callExpression(
            t.identifier("listConciliar"),
            [
                parentElement,
                array,
                callbackFunction
            ]
        );

        // Gather all state dependencies for the effect
        const allDeps = new Set<t.Expression>();

        // Add array dependency if it's state-dependent
        if (stateDependency) {
            allDeps.add(t.identifier(stateDependency));
        }

        // Add all state variables used in the callback
        stateVars.forEach(varName => {
            allDeps.add(t.identifier(varName));
        });

        // Create the effect
        const effectCall = t.callExpression(
            t.identifier("effect"),
            [
                t.arrowFunctionExpression(
                    [],
                    t.blockStatement([t.expressionStatement(concileCall)])
                ),
                t.arrayExpression(Array.from(allDeps))
            ]
        );
        funcState.specialFlags.insideMapHandler = false;
        return t.expressionStatement(effectCall);
    } else {
        // For completely static maps (no state dependencies),
        // we can use a simpler approach without reactivity
        const fixedCallback = ensureReturnInMapCallback(callback as t.ArrowFunctionExpression);

        // Create a single-time map operation during initial render
        const mapCall = t.callExpression(
            t.memberExpression(array, t.identifier("map")),
            [fixedCallback]
        );

        // Add all mapped items to the parent at once
        const addChildrenCall = t.callExpression(
            t.memberExpression(parentElement, t.identifier("addChildren")),
            [mapCall]
        );
        funcState.specialFlags.insideMapHandler = false;
        return t.expressionStatement(addChildrenCall);
    }
}

type DynamicStyle = [string, t.Expression, string];
type Item = ["style" | "prop" | "spread", string | null, t.Expression, string | null]; //type, propName, value, usedState
type DynamicUpdate = Array<Item>

function processAttrs(
    attrs: Array<t.JSXAttribute | t.JSXSpreadAttribute>,
    path: NodePath,
    functionScope: FunctionScope, forceStatic: boolean
): [t.ObjectExpression, DynamicUpdate] {
    const props = t.objectExpression([]);
    const dynamicUpdates: DynamicUpdate = [];

    attrs.forEach(attr => {
        if (t.isJSXAttribute(attr)) {
            const propName = attr.name.name as string;
            let propValue: t.Expression;

            if (attr.value === null) {
                // Handle boolean attributes (no value)
                propValue = t.booleanLiteral(true);
            } else if (t.isJSXExpressionContainer(attr.value)) {
                propValue = attr.value.expression as t.Expression;

                // Process object style attributes specially
                if (propName === 'style' && t.isObjectExpression(propValue)) {
                    const styleProps = propValue.properties;
                    const staticStyles: t.ObjectProperty[] = [];
                    const dynamicStyles: DynamicStyle[] = [];

                    // Separate static and dynamic style properties
                    styleProps.forEach(styleProp => {
                        if (t.isObjectProperty(styleProp)) {
                            const styleValue = styleProp.value as t.Expression;
                            let styleName: LiteralType;

                            if (t.isIdentifier(styleProp.key)) {
                                styleName = styleProp.key.name;
                            } else if (t.isStringLiteral(styleProp.key)) {
                                styleName = styleProp.key.value;
                            } else {
                                throw new Error(`Unrecognized style key type`);
                            }
                            const usedState = containsStateGetterCall(styleValue, functionScope);
                            if (usedState && !forceStatic) {
                                dynamicStyles.push([styleName.toString(), styleValue, usedState]);
                            } else {
                                staticStyles.push(styleProp as t.ObjectProperty);
                            }
                        }
                    });

                    // Add static styles to props
                    propValue = t.objectExpression(staticStyles);

                    // Track dynamic styles for effects
                    dynamicStyles.forEach(([styleName, styleValue, usedStateVariable]: DynamicStyle) => {
                        const item: Item = ['style', styleName, styleValue, usedStateVariable];
                        dynamicUpdates.push(item);
                    });
                } else if (t.isJSXEmptyExpression(propValue) && !forceStatic) {
                    dynamicUpdates.push(['prop', propName, t.booleanLiteral(true), null]);
                } else {
                    const stateID = containsStateGetterCall(propValue, functionScope)
                    if (stateID) {
                        dynamicUpdates.push(['prop', propName, propValue, stateID]);
                    }
                }

            } else if (t.isStringLiteral(attr.value) || t.isNumericLiteral(attr.value)) {
                propValue = attr.value;
            } else {
                // Default to boolean true for attributes without values
                propValue = t.booleanLiteral(true);
            }

            props.properties.push(t.objectProperty(t.identifier(propName), propValue));
        } else if (t.isJSXSpreadAttribute(attr)) {
            // Handle spread attributes like {...props}
            const spreadArgument = attr.argument;

            // Add the spread properties to the props object
            props.properties.push(
                t.spreadElement(spreadArgument)
            );

            // If the spread contains state getters, track it for dynamic updates
            const stateID = containsStateGetterCall(spreadArgument, functionScope);
            if (stateID && !forceStatic) {
                // Create a special spread type dynamic update
                // The spread needs to be tracked as a whole object
                dynamicUpdates.push(['spread', null, spreadArgument, stateID]);
            }

            // If it's an object expression, we can analyze its properties individually
            if (t.isObjectExpression(spreadArgument)) {
                spreadArgument.properties.forEach(prop => {
                    if (t.isObjectProperty(prop) && t.isExpression(prop.value)) {
                        const key = getPropertyKey(prop);
                        const stateID = containsStateGetterCall(prop.value, functionScope);
                        if (key && stateID) {
                            dynamicUpdates.push(['prop', key, prop.value, stateID]);
                        }
                    }
                });
            }
        }

    });

    return [props, dynamicUpdates];
}


//returns true if the component is static component
interface JsxResult {
    expression: t.Expression | t.SpreadElement | null;
    statements: t.Statement[];
    deps?: t.Identifier[]
}

/**
 * Handle JSX expressions such as {condition && <Component/>} or {condition ? <A/> : <B/>}
 */

export function handleJsxExpression(
    path: NodePath<t.JSXExpressionContainer>,
    funcState: FunctionScope,
    parentElement: t.Identifier,
    isParentText: boolean
): { statements: t.Statement[], deps: t.Identifier[] } {
    const expression = path.node.expression;
    const statements: t.Statement[] = [];
    const deps: t.Identifier[] = [];
    const expressionId = generateShortId();

    // Skip empty expressions like {}
    if (t.isJSXEmptyExpression(expression)) {
        return {statements, deps};
    }

    // Handle logical expressions: condition && <Component/>, condition || <Component/>, condition ?? <Component/>
    if (t.isLogicalExpression(expression)) {
        return handleLogicalExpression(
            expression,
            path,
            funcState,
            parentElement,
            isParentText,
            expressionId,
            statements,
            deps
        );
    }
    // Handle ternary expressions: condition ? <A/> : <B/>
    else if (t.isConditionalExpression(expression)) {
        return handleConditionalExpression(
            expression,
            path,
            funcState,
            parentElement,
            isParentText,
            expressionId,
            statements,
            deps
        );
    }
    // Handle other expressions (like simple variables or function calls)
    else {
        return handleSimpleExpression(
            expression,
            funcState,
            parentElement,
            isParentText,
            expressionId,
            statements,
            deps
        );
    }
}

/**
 * Handles logical expressions like &&, ||, ??
 */
function handleLogicalExpression(
    expression: t.LogicalExpression,
    path: NodePath<t.JSXExpressionContainer>,
    funcState: FunctionScope,
    parentElement: t.Identifier,
    isParentText: boolean,
    expressionId: string,
    statements: t.Statement[],
    deps: t.Identifier[]
): { statements: t.Statement[], deps: t.Identifier[] } {
    const {operator, left: condition, right: content} = expression;

    // Get state dependencies from the condition
    const stateDependency = containsStateGetterCall(condition, funcState);
    if (stateDependency) {
        deps.push(t.identifier(stateDependency));
    }

    if (t.isJSXElement(content)) {
        return handleLogicalJsxContent(
            operator,
            condition,
            content,
            path,
            funcState,
            parentElement,
            expressionId,
            stateDependency,
            statements,
            deps
        );
    } else {
        return handleLogicalNonJsxContent(
            operator,
            condition,
            content,
            funcState,
            parentElement,
            expressionId,
            isParentText,
            stateDependency,
            statements,
            deps
        );
    }
}

/**
 * Handles JSX content in logical expressions
 */
function handleLogicalJsxContent(
    operator: string,
    condition: t.Expression,
    content: t.JSXElement,
    path: NodePath<t.JSXExpressionContainer>,
    funcState: FunctionScope,
    parentElement: t.Identifier,
    expressionId: string,
    stateDependency: string | null,
    statements: t.Statement[],
    deps: t.Identifier[]
): { statements: t.Statement[], deps: t.Identifier[] } {
    // Transform the JSX element
    const jsxResult = handleJsxElement(
        path.get('expression.right') as NodePath<t.JSXElement>,
        funcState,
        undefined,
        true
    );

    if (jsxResult.expression) {
        // Create placeholder in the parent element with a unique ID
        const placeholderId = t.stringLiteral(expressionId);

        // Create the condition expression based on the operator
        let conditionExpr = condition;

        // For || and ??, the condition needs to be inverted
        if (operator === '||' || operator === '??') {
            // Show content when condition is falsy: !condition
            conditionExpr = t.unaryExpression('!', condition);
        }
        // For && the condition remains as is: condition

        // Create effect to update the conditional rendering
        const effectBody = t.blockStatement([
            t.ifStatement(
                conditionExpr,
                t.blockStatement([
                    t.expressionStatement(
                        t.callExpression(
                            t.memberExpression(parentElement, t.identifier('insertChild')),
                            [placeholderId, jsxResult.expression]
                        )
                    )
                ]),
                t.blockStatement([
                    t.expressionStatement(
                        t.callExpression(
                            t.memberExpression(parentElement, t.identifier('removeChild')),
                            [placeholderId]
                        )
                    )
                ])
            )
        ]);

        // Create the effect with appropriate dependencies
        const effectDeps = stateDependency ? [t.identifier(stateDependency)] : [];
        const effectCall = t.callExpression(
            t.identifier('effect'),
            [
                t.arrowFunctionExpression([], effectBody),
                t.arrayExpression(effectDeps)
            ]
        );

        statements.push(...jsxResult.statements);
        statements.push(t.expressionStatement(effectCall));

        // Collect dependencies from jsxResult if available
        if (jsxResult.deps) {
            deps.push(...jsxResult.deps);
        }
    }

    return {statements, deps};
}

/**
 * Handles non-JSX content in logical expressions (variables, function calls, etc.)
 */
function handleLogicalNonJsxContent(
    operator: string,
    condition: t.Expression,
    content: t.Expression,
    funcState: FunctionScope,
    parentElement: t.Identifier,
    expressionId: string,
    isParentText: boolean,
    stateDependency: string | null,
    statements: t.Statement[],
    deps: t.Identifier[]
): { statements: t.Statement[], deps: t.Identifier[] } {
    const contentDep = containsStateGetterCall(content, funcState);

    if (contentDep) {
        if (funcState.foundChildren && contentDep === "children") {
            // Create the condition expression based on the operator
            let conditionExpr = condition;

            // For || and ??, the condition needs to be inverted
            if (operator === '||') {
                // Show content when condition is falsy: !condition
                conditionExpr = t.unaryExpression('!', condition);
            } else if (operator === '??') {
                // Show content when condition is null/undefined: condition == null
                conditionExpr = t.binaryExpression(
                    '==',
                    condition,
                    t.nullLiteral()
                );
            }
            // For && the condition remains as is: condition

            // Create effect to update the conditional rendering
            const effectBody = t.blockStatement([
                t.ifStatement(
                    conditionExpr,
                    t.blockStatement([
                        t.expressionStatement(
                            t.callExpression(
                                t.memberExpression(parentElement, t.identifier('insertChildren')),
                                [t.identifier("children")]
                            )
                        )
                    ]),
                    t.blockStatement([
                        t.expressionStatement(
                            t.callExpression(
                                t.memberExpression(parentElement, t.identifier('removeChildren')),
                                []
                            )
                        )
                    ])
                )
            ]);

            // Create the effect with appropriate dependencies
            const effectDeps = stateDependency ? [t.identifier(stateDependency)] : [];
            const effectCall = t.callExpression(
                t.identifier('effect'),
                [
                    t.arrowFunctionExpression([], effectBody),
                    t.arrayExpression(effectDeps)
                ]
            );

            statements.push(t.expressionStatement(effectCall));
        }

        if (stateDependency) {
            deps.push(t.identifier(stateDependency));
        }

        if (contentDep && contentDep !== "children") {
            deps.push(t.identifier(contentDep));
        }
    }

    return {statements, deps};
}

/**
 * Handles conditional (ternary) expressions: condition ? <A/> : <B/>
 */
function handleConditionalExpression(
    expression: t.ConditionalExpression,
    path: NodePath<t.JSXExpressionContainer>,
    funcState: FunctionScope,
    parentElement: t.Identifier,
    isParentText: boolean,
    expressionId: string,
    statements: t.Statement[],
    deps: t.Identifier[]
): { statements: t.Statement[], deps: t.Identifier[] } {
    const condition = expression.test;
    const consequent = expression.consequent;
    const alternate = expression.alternate;

    // Get state dependencies from the condition
    const stateDependency = containsStateGetterCall(condition, funcState);
    if (stateDependency) {
        deps.push(t.identifier(stateDependency));
    }

    // Handle JSX in both branches
    let consequentResult = handleConditionalBranch(
        consequent,
        path.get('expression.consequent') as NodePath<any>,
        funcState,
        isParentText
    );

    let alternateResult = handleConditionalBranch(
        alternate,
        path.get('expression.alternate') as NodePath<any>,
        funcState,
        isParentText
    );

    // Collect dependencies from both branches
    if (consequentResult.deps) {
        deps.push(...consequentResult.deps);
    }

    if (alternateResult.deps) {
        deps.push(...alternateResult.deps);
    }

    // Create placeholder in the parent element with a unique ID
    const placeholderId = t.stringLiteral(expressionId);

    const effectBodyStatements = [];

    // Add condition branch
    if (consequentResult.expression) {
        effectBodyStatements.push(
            t.ifStatement(
                condition,
                t.blockStatement([
                    t.expressionStatement(
                        t.callExpression(
                            t.memberExpression(parentElement, t.identifier('insertChild')),
                            [placeholderId, consequentResult.expression]
                        )
                    )
                ]),
                t.blockStatement([
                    // If alternate exists, insert it, otherwise remove the child
                    alternateResult.expression
                        ? t.expressionStatement(
                            t.callExpression(
                                t.memberExpression(parentElement, t.identifier('insertChild')),
                                [placeholderId, alternateResult.expression]
                            )
                        )
                        : t.expressionStatement(
                            t.callExpression(
                                t.memberExpression(parentElement, t.identifier('removeChild')),
                                [placeholderId]
                            )
                        )
                ])
            )
        );
    }

    // Create the effect with appropriate dependencies
    const effectDeps = stateDependency ? [t.identifier(stateDependency)] : [];
    const effectCall = t.callExpression(
        t.identifier('effect'),
        [
            t.arrowFunctionExpression([], t.blockStatement(effectBodyStatements)),
            t.arrayExpression(effectDeps)
        ]
    );

    // Add all statements from both branches
    statements.push(...consequentResult.statements);
    statements.push(...alternateResult.statements);
    statements.push(t.expressionStatement(effectCall));

    return {statements, deps};
}

/**
 * Handles a branch of a conditional expression (consequent or alternate)
 */
function handleConditionalBranch(
    expression: t.Expression,
    path: NodePath<any>,
    funcState: FunctionScope,
    isParentText: boolean
): JsxResult {
    if (t.isJSXElement(expression)) {
        return handleJsxElement(
            path as NodePath<t.JSXElement>,
            funcState,
            undefined,
            true
        );
    } else if (!t.isJSXEmptyExpression(expression)) {
        // Handle non-JSX expressions
        let expr: t.Expression;
        const deps: t.Identifier[] = [];

        if (isParentText || (funcState.foundChildren && t.isIdentifier(expression, {name: "children"}))) {
            expr = expression;
        } else {
            expr = createTextComponentObject(expression);
        }

        // Check for dependencies in the expression
        const expressionDep = containsStateGetterCall(expression, funcState);
        if (expressionDep) {
            deps.push(t.identifier(expressionDep));
        }

        return {
            expression: expr,
            statements: [],
            deps
        };
    }

    return {
        expression: null,
        statements: [],
        deps: []
    };
}

/**
 * Creates a text component object for non-JSX expressions
 */
function createTextComponentObject(content: t.Expression): t.ObjectExpression {
    return t.objectExpression([
        t.objectProperty(t.identifier('$$internalComponent'), t.booleanLiteral(true)),
        t.objectProperty(t.identifier('component'), t.stringLiteral('text')),
        t.objectProperty(
            t.identifier('props'),
            t.objectExpression([
                t.objectProperty(
                    t.identifier('children'),
                    t.arrayExpression([content])
                )
            ])
        ),
        t.objectProperty(t.identifier('id'), t.stringLiteral(generateShortId()))
    ]);
}

/**
 * Handles simple expressions like variables or function calls
 */
function handleSimpleExpression(
    expression: t.Expression,
    funcState: FunctionScope,
    parentElement: t.Identifier,
    isParentText: boolean,
    expressionId: string,
    statements: t.Statement[],
    deps: t.Identifier[]
): { statements: t.Statement[], deps: t.Identifier[] } {
    const stateDependency = containsStateGetterCall(expression, funcState);

    if (stateDependency) {
        deps.push(t.identifier(stateDependency));

        // Create placeholder with unique ID
        const placeholderId = t.stringLiteral(expressionId);

        let expr: t.Expression;
        const isChildren = (funcState.foundChildren && t.isIdentifier(expression, {name: "children"}))
        if (isParentText || isChildren) {
            expr = expression;
        } else {
            expr = createTextComponentObject(expression);
        }

        // Effect to update text content
        let effectBody: t.BlockStatement
        if (isChildren) {
            effectBody = t.blockStatement([
                t.expressionStatement(
                    t.callExpression(
                        t.memberExpression(parentElement, t.identifier('insertChildren')),
                        [expression]
                    )
                )
            ]);
        } else {
            effectBody = t.blockStatement([
                t.expressionStatement(
                    t.callExpression(
                        t.memberExpression(parentElement, t.identifier('insertChild')),
                        [placeholderId, expr]
                    )
                )
            ]);
        }


        // Create the effect with appropriate dependencies
        const effectCall = t.callExpression(
            t.identifier('effect'),
            [
                t.arrowFunctionExpression([], effectBody),
                t.arrayExpression(isChildren ? []:[t.identifier(stateDependency)])
            ]
        );

        statements.push(t.expressionStatement(effectCall));
    } else {
        // For static content, directly insert it
        statements.push(
            t.expressionStatement(
                t.callExpression(
                    t.memberExpression(parentElement, t.identifier('addStaticChild')),
                    [createTextComponentObject(expression)]
                )
            )
        );
    }

    return {statements, deps};
}

export function handleJsxElement(
    path: NodePath<t.JSXElement>,
    funcState: FunctionScope,
    parentVariable?: t.Identifier,
    forceStatic = false,
    canCreateHolder = true
): JsxResult {
    const isInReturnStatement = path.parentPath && path.parentPath.isReturnStatement();
    const isVariable = path.parentPath.isVariableDeclarator();

    const elementName = getJsxElementName(path.node.openingElement);
    const isInternal = INTERNAL_COMPONENTS.includes(elementName);
    const props = path.node.openingElement.attributes as t.JSXAttribute[];
    const originalForceStatic = forceStatic;
    if (!forceStatic) {
        forceStatic = !isInternal && parentVariable !== undefined;
    }
    let elementVariable: Identifier;
    if (isVariable) {
        elementVariable = (path.parentPath.node as t.VariableDeclarator).id as t.Identifier;
    } else
        elementVariable = path.scope.generateUidIdentifier(parentVariable ? "element" : "parent");
    // Process attributes
    const [staticProps, dynamicProps] = processAttrs(props, path, funcState, false);


    // Process children
    const childrenResults: JsxResult[] = [];

    path.node.children.forEach((child, index) => {
        if (t.isJSXElement(child)) {
            const childPath = path.get('children')[index] as NodePath<t.JSXElement>;
            const result = handleJsxElement(childPath, funcState, elementVariable, !isInternal);
            childrenResults.push(result);
        } else if (t.isJSXText(child)) {
            const text = child.value.trim();
            if (text) {
                childrenResults.push({expression: t.stringLiteral(text), statements: []});
            }
        } else if (t.isJSXExpressionContainer(child)) {
            const exp = child.expression;
            if (t.isJSXEmptyExpression(exp)) {
                // Handle empty expression by using an empty string literal
                childrenResults.push({expression: t.stringLiteral(""), statements: []});
            } else if (t.isCallExpression(exp) && isMapExpression(exp)) {
                if (funcState.specialFlags.insideMapHandler) {
                    const a = t.spreadElement(exp);
                    childrenResults.push({expression: a, statements: []})
                } else {
                    forceStatic = originalForceStatic;
                    const mapParent = path.scope.generateUidIdentifier("mapParent");
                    const mapInfo = extractMapInfo(exp as t.CallExpression, funcState, path);
                    if (mapInfo) {
                        const createCall = t.callExpression(t.identifier("createElement"), [t.stringLiteral('component'), t.objectExpression([])]);
                        const declaration = t.variableDeclaration('const', [
                            t.variableDeclarator(mapParent, createCall)
                        ]);
                        const mapHandler = createMapHandler(mapInfo, path.get('children')[index], funcState, mapParent, path);
                        const statements = [declaration, mapHandler];
                        childrenResults.push({expression: mapParent, statements});
                    }
                }

            }
            // Handle logical expressions (&&) and conditional expressions (ternary)
            else if (t.isLogicalExpression(exp) && exp.operator === '&&' && t.isJSXElement(exp.right)) {
                forceStatic = originalForceStatic;
                // Let the specialized handler deal with this
                const expressionStatements = handleJsxExpression(
                    path.get(`children.${index}`) as NodePath<t.JSXExpressionContainer>,
                    funcState,
                    elementVariable,
                    elementName === "text"
                );
                childrenResults.push({expression: null, ...expressionStatements});
            } else if (t.isConditionalExpression(exp) &&
                (t.isJSXElement(exp.consequent) || t.isJSXElement(exp.alternate))) {
                forceStatic = originalForceStatic;
                // Let the specialized handler deal with this
                const expressionStatements = handleJsxExpression(
                    path.get(`children.${index}`) as NodePath<t.JSXExpressionContainer>,
                    funcState,
                    elementVariable,
                    elementName === "text"
                );
                childrenResults.push({expression: null, ...expressionStatements});
            } else {
                forceStatic = originalForceStatic;
                const variables = containsStateGetterCall(exp, funcState);
                if (variables && !forceStatic) {
                    const expressionStatements = handleJsxExpression(
                        path.get(`children.${index}`) as NodePath<t.JSXExpressionContainer>,
                        funcState,
                        elementVariable,
                        elementName === "text"
                    );
                    childrenResults.push({expression: null, ...expressionStatements});
                } else {
                    const result: JsxResult = {expression: exp, statements: []};
                    if (variables) {
                        result.deps = [t.identifier(variables)];
                    }
                    childrenResults.push(result);
                }
            }
        }
    });

    // Determine if the element can be static
    const canBeStatic = forceStatic || (
        dynamicProps.length === 0 &&
        childrenResults.every(result => result.expression !== null && result.statements.length === 0)
        && parentVariable !== undefined
    );

    if (canBeStatic) {
        const childrenExpressions = childrenResults.map(result => result.expression!).filter(Boolean);
        const childrenStatement = childrenResults.flatMap(result => result.statements!).filter(Boolean);
        staticProps.properties.push(createObjectProperty("children", t.arrayExpression(childrenExpressions)));
        const staticObject = t.objectExpression([
            createObjectProperty("$$internalComponent", t.booleanLiteral(isInternal)),
            createObjectProperty("component", isInternal ? t.stringLiteral(elementName) : t.identifier(elementName)),
            createObjectProperty("props", staticProps),
            createObjectProperty("id", t.stringLiteral(generateShortId()))
        ]);
        if (originalForceStatic && canCreateHolder && dynamicProps.length > 0) {
            const holder = path.scope.generateUidIdentifier("holder")
            const caller = t.callExpression(t.identifier(`createElement`), [t.stringLiteral("holder"), t.objectExpression([])])
            const definition = t.variableDeclaration("const", [t.variableDeclarator(holder, caller)])
            childrenStatement.push(definition)
            const updater = t.callExpression(t.memberExpression(holder, t.identifier("setChild")), [staticObject]);
            const arrowFunction = t.arrowFunctionExpression([], t.blockStatement([t.expressionStatement(updater)]));
            const depsExpression = t.arrayExpression(dynamicProps.map((v) => t.identifier(v[3]!)));
            const effect = t.callExpression(t.identifier("effect"), [arrowFunction, depsExpression]);
            return {expression: holder, statements: [...childrenStatement, t.expressionStatement(effect)]};
        }
        const depsArray = dynamicProps.map((v) => t.identifier(v[3]!));
        const childrenDeps = childrenResults.flatMap(result => result.deps || []);
        depsArray.push(...childrenDeps);
        if (depsArray.length === 0 || originalForceStatic) {
            return {expression: staticObject, statements: childrenStatement, deps: depsArray};
        }

        if (parentVariable) {

            const updater = t.callExpression(t.memberExpression(parentVariable, t.identifier("insertChild")), [
                t.stringLiteral(generateShortId()),
                staticObject
            ]);
            const arrowFunction = t.arrowFunctionExpression([], t.blockStatement([t.expressionStatement(updater)]));
            const depsExpression = t.arrayExpression(depsArray);
            const effect = t.callExpression(t.identifier("effect"), [arrowFunction, depsExpression]);
            return {expression: null, statements: [...childrenStatement, t.expressionStatement(effect)]};
        }
        return {expression: staticObject, statements: childrenStatement};
    }

    // Dynamic case
    const statements: t.Statement[] = [];


    const object = isInternal
        ? t.callExpression(t.identifier(`createElement`), [t.stringLiteral(elementName), staticProps])
        : t.callExpression(t.identifier(elementName), [staticProps]);
    statements.push(t.variableDeclaration('const', [t.variableDeclarator(elementVariable, object)]));
    dynamicProps.forEach(prop => {
        let updater: t.Statement;
        if (prop[0] === "prop") {
            updater = t.expressionStatement(t.assignmentExpression(
                "=",
                t.memberExpression(elementVariable, t.identifier(prop[1]!)),
                prop[2]
            ));
        } else if (prop[0] === "style") {
            updater = t.expressionStatement(t.assignmentExpression(
                "=",
                t.memberExpression(t.memberExpression(elementVariable, t.identifier("style")), t.identifier(prop[1]!)),
                prop[2]
            ));
        } else if (prop[0] === "spread") {
            updater = t.expressionStatement(
                t.callExpression(
                    t.memberExpression(elementVariable, t.identifier("setProps")),
                    [prop[2]]
                )
            );
        } else {
            throw new Error(`Prop ${prop[0]} is not implemented yet`);
        }
        const arrowFunction = t.arrowFunctionExpression([], t.blockStatement([updater]));
        const depsExpression = t.arrayExpression([t.identifier(prop[3]!)]);
        const effect = t.callExpression(t.identifier("effect"), [arrowFunction, depsExpression]);
        statements.push(t.expressionStatement(effect));
    });

    childrenResults.forEach(result => {
        statements.push(...result.statements)
        if (result.expression) {
            const method = t.isObjectExpression(result.expression) ? "addStaticChild" : "addChild"
            const addCall = t.callExpression(
                t.memberExpression(elementVariable, t.identifier(method)),
                [result.expression]
            );
            statements.push(t.expressionStatement(addCall));
        }

    });

    if (isInReturnStatement) {
        const endCall = t.expressionStatement(t.callExpression(t.identifier("endComponent"), []));
        const parent = path.findParent(p => p.isFunction());
        if (parent) statements.push(endCall);
        statements.push(t.returnStatement(elementVariable));
        path.parentPath.replaceWith(t.blockStatement(statements));
        return {expression: null, statements: []};
    } else if (parentVariable) {
        statements.push(t.expressionStatement(
            t.callExpression(
                t.memberExpression(parentVariable, t.identifier('addChild')),
                [elementVariable]
            )
        ));
        return {expression: null, statements};
    } else if (isVariable) {
        //Need to return the very parent
        path.parentPath.parentPath?.replaceWithMultiple(statements)
        return {expression: null, statements: []};
    }

    return {expression: elementVariable, statements};
}


function findAvailableVariables(node: t.Node, functionScope: FunctionScope, path: NodePath): string[] {
    if (functionScope.variables.size === 0) {
        return [];
    }

    const stateVars = new Set<string>();

    // Traverse AST to find state variables
    try {
        traverse(node, {
            Identifier(path) {
                if (functionScope.variables.has(path.node.name)) {
                    stateVars.add(path.node.name);
                }
            },
            MemberExpression(path) {
                const {node} = path;
                if (t.isIdentifier(node.object, {name: 'state'}) &&
                    t.isIdentifier(node.property) &&
                    functionScope.variables.has(node.property.name)) {
                    stateVars.add(node.property.name);
                }
            }
        }, path.scope);
    } catch (e) {
        console.warn("Error finding state variables:", e);
    }

    return Array.from(stateVars);
}
