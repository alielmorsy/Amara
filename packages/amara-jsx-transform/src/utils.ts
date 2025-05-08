import crypto from "crypto";
import {type NodePath, traverse, types as t} from '@babel/core'
import {FunctionScope} from "./transform/types";

export const INTERNAL_COMPONENTS = ['div', 'Button', 'button', 'ToggleButton', 'ul', 'li', 'h2', 'h1', 'lelo', 'span', 'text', 'holder', 'HOLDER_ELEMENT'];

export function generateShortId(length = 12) {
    return crypto.randomBytes(Math.ceil(length / 2)).toString('base64').replace(/[^a-zA-Z0-9]/g, '').slice(0, length);
}

export function getPropertyKey(prop: t.ObjectProperty): string | undefined {
    if (t.isIdentifier(prop.key)) {
        return prop.key.name;
    } else if (t.isStringLiteral(prop.key)) {
        return prop.key.value;
    } else if (t.isNumericLiteral(prop.key)) {
        return prop.key.value.toString();
    }
}

export function createObjectProperty(name: string, value: t.Expression | t.ArrayExpression) {
    return t.objectProperty(t.stringLiteral(name), value)
}

export function isMapExpression(expr: t.Node) {
    return t.isCallExpression(expr) &&
        t.isMemberExpression(expr.callee) && t.isIdentifier(expr.callee.property) &&
        expr.callee.property.name === 'map';
}

/**
 * Checks if a map expression is mapping over components
 * This helps avoid creating unnecessary holders for component maps
 */
export function isComponentMap(node: t.Node): boolean {
    if (!isMapExpression(node) || !t.isCallExpression(node)) {
        return false;
    }

    // Check the callback function of the map
    const mapCallback = node.arguments[0];
    if (!t.isArrowFunctionExpression(mapCallback) && !t.isFunctionExpression(mapCallback)) {
        return false;
    }

    // Check the body of the callback
    const body = mapCallback.body;
    if (t.isBlockStatement(body)) {
        // Look for return statements with JSX elements
        for (const stmt of body.body) {
            if (t.isReturnStatement(stmt) && stmt.argument &&
                (t.isJSXElement(stmt.argument) ||
                    (t.isCallExpression(stmt.argument) &&
                        t.isIdentifier(stmt.argument.callee) &&
                        /^[A-Z]/.test(stmt.argument.callee.name)))) {
                return true;
            }
        }
        return false;
    } else if (t.isJSXElement(body) ||
        (t.isCallExpression(body) &&
            t.isIdentifier(body.callee) &&
            /^[A-Z]/.test(body.callee.name))) {
        // Direct return of a JSX element or component call
        return true;
    }

    return false;
}

export function containsStateGetterCall(node: t.Node, functionScope: FunctionScope) {


    if (!node || functionScope.variables.size === 0) {
        return null;
    }

    let hasStateGetter: string | null = null;

    //It's just a function which hold the proxy reference
    if (t.isCallExpression(node)) {
        return null;
    }
    // Check if the node is a direct reference to a state getter
    if (t.isIdentifier(node)) {
        if (functionScope.variables.has(node.name))
            return node.name;
        else {
            return null
        }

    }


    // Use proper traversal to look for state getters
    try {
        const wrapped = t.file(
            t.program([
                t.expressionStatement(node as t.Expression),
            ])
        );
        traverse(wrapped, {
            Identifier(path) {
                if (hasStateGetter) return;
                if (functionScope.variables.has(path.node.name)) {
                    hasStateGetter = path.node.name;
                    path.stop(); // Stop traversal once found
                }
            },
            MemberExpression(path) {
                if (hasStateGetter) return;

                const {node} = path;
                if (t.isIdentifier(node.object, {name: 'state'}) &&
                    t.isIdentifier(node.property) &&
                    functionScope.variables.has(node.property.name)) {
                    hasStateGetter = node.property.name;
                    path.stop(); // Stop traversal once found
                }
            },
            CallExpression(path) {
                const node = path.node;
                if (t.isIdentifier(node.callee)) {
                    const name = node.callee.name;
                    if (functionScope.variables.has(name)) {
                        hasStateGetter = name;
                        path.stop(); // Stop traversal once found
                    }
                    return
                }
            }
        });
    } catch (e) {
        // Handle traversal error - this can happen with complex expressions
        console.warn("Error traversing node for state getters:", e);
    }

    return hasStateGetter;
}

export interface MapInfo {
    array: t.Expression;
    callback: t.ArrowFunctionExpression | t.FunctionExpression;
    itemParam: t.Identifier[];
    indexParam: string | null;
    stateDependency: string | null;
    stateVars: string[];
    callbackBody: t.BlockStatement | t.Expression;
}


export function extractMapInfo(mapExpr: t.CallExpression, functionScope: FunctionScope, path: NodePath): MapInfo | null {
    try {
        const array = mapExpr.callee;
        const callback = mapExpr.arguments[0] as t.ArrowFunctionExpression | t.FunctionExpression;

        if (!callback || (!t.isArrowFunctionExpression(callback) && !t.isFunctionExpression(callback))) {
            return null;
        }

        // Extract callback parameters
        const itemParam = callback.params as t.Identifier[];
        const indexParam = callback.params[1] ? (callback.params[1] as t.Identifier).name : null;

        // Check if array is state-dependent
        const isStateDependentArray = containsStateGetterCall(array, functionScope);

        // Extract state dependencies inside the callback body
        const stateVars: string[] = findAvailableVariables(callback.body, functionScope, path);

        return {
            array: array as t.Expression,
            callback,
            itemParam,
            indexParam,
            stateDependency: isStateDependentArray,
            stateVars,
            callbackBody: callback.body
        };
    } catch (e) {
        console.warn("Error extracting map info:", e);
        return null;
    }
}

function findAvailableVariables(node: t.Statement | t.Expression, functionScope: FunctionScope, path: NodePath): string[] {
    if (functionScope.variables.size === 0) {
        return [];
    }

    const stateVars = new Set<string>();

    let useNode: t.Statement;
    if (!t.isStatement(node)) {
        useNode = t.expressionStatement(node as unknown as t.Expression);
    } else {
        useNode = node
    }
    const tNode = t.file(t.program([useNode]));
    try {
        traverse(tNode, {
            Identifier(path) {
                if (functionScope.variables.has(path.node.name)) {
                    stateVars.add(path.node.name);
                }
            },
            MemberExpression(path) {
                const {node} = path;
                if (
                    t.isIdentifier(node.object, {name: 'state'}) &&
                    t.isIdentifier(node.property) &&
                    functionScope.variables.has(node.property.name)
                ) {
                    stateVars.add(node.property.name);
                }
            }
        });
    } catch (e) {
        console.warn("Error finding state variables:", e);
    }

    return Array.from(stateVars);
}

function transformMapCallbackBody(body: t.Expression | t.BlockStatement): t.Expression {
    // For block statements, process each statement
    if (t.isBlockStatement(body)) {
        // Get the return statement or last expression
        const statements = body.body;
        const lastStmt = statements[statements.length - 1];

        if (t.isReturnStatement(lastStmt) && lastStmt.argument) {
            // Process the return value
            return lastStmt.argument;
        } else {
            // If no explicit return, create a placeholder expression
            return t.identifier('undefined');
        }
    }
    // For expression bodies in arrow functions
    else {
        return body;
    }
}

export function ensureReturnInMapCallback(callback: t.ArrowFunctionExpression): t.ArrowFunctionExpression {
    if (!t.isBlockStatement(callback.body)) {
        // Implicit return case
        if (t.isJSXElement(callback.body)) {
            return callback;
        } else if (
            t.isCallExpression(callback.body) &&
            t.isFunction(callback.body.callee) &&
            t.isBlockStatement((callback.body.callee as t.ArrowFunctionExpression | t.FunctionExpression).body)
        ) {
            callback.body = (callback.body.callee as t.ArrowFunctionExpression | t.FunctionExpression).body;
            return callback;
        }
        return callback;
    }

    // Ensure the last statement in a block is a return
    const statements = callback.body.body;
    const lastStmt = statements[statements.length - 1];

    if (!t.isReturnStatement(lastStmt) && statements.length > 0) {
        if (t.isExpressionStatement(lastStmt)) {
            statements[statements.length - 1] = t.returnStatement(lastStmt.expression);
        }
    }

    return callback;
}


function transformExpressionToObject(expr: t.Expression, valueParam: t.Identifier, indexParam: t.Identifier): t.Expression {
    if (t.isJSXElement(expr)) {
        // Extract key from JSX if available
        const keyAttr = expr.openingElement.attributes.find(
            attr => t.isJSXAttribute(attr) && (attr.name as t.JSXIdentifier).name === 'key'
        ) as t.JSXAttribute | undefined;

        // Get element name
        const elementName = (expr.openingElement.name as t.JSXIdentifier).name;
        const isInternal = INTERNAL_COMPONENTS.includes(elementName);

        // Create object structure for our element
        return t.objectExpression([
            createObjectProperty("$$internalComponent", t.booleanLiteral(isInternal)),
            createObjectProperty("component", isInternal ? t.stringLiteral(elementName) : t.identifier(elementName)),
            createObjectProperty("props", t.objectExpression([])),  // Props would be processed fully in real implementation
            createObjectProperty("id", t.stringLiteral(generateShortId())),
            createObjectProperty("key", keyAttr && keyAttr.value
                ? t.isJSXExpressionContainer(keyAttr.value)
                    ? keyAttr.value.expression as t.Expression
                    : t.stringLiteral((keyAttr.value as t.StringLiteral).value)
                : indexParam
            )
        ]);
    }

    // Return the expression directly if it's not JSX
    return expr;
}

function transformCallbackToObject(body: t.BlockStatement, valueParam: t.Identifier, indexParam: t.Identifier): t.Expression {
    // Find return statement in the block
    const returnStmt = body.body.find(stmt => t.isReturnStatement(stmt)) as t.ReturnStatement | undefined;

    if (returnStmt && returnStmt.argument) {
        if (t.isJSXElement(returnStmt.argument)) {
            // Extract key from JSX if available
            const keyAttr = returnStmt.argument.openingElement.attributes.find(
                attr => t.isJSXAttribute(attr) && (attr.name as t.JSXIdentifier).name === 'key'
            ) as t.JSXAttribute | undefined;

            // Get element name
            const elementName = (returnStmt.argument.openingElement.name as t.JSXIdentifier).name;
            const isInternal = INTERNAL_COMPONENTS.includes(elementName);

            // Create a reference to our element creation system
            return t.objectExpression([
                createObjectProperty("$$internalComponent", t.booleanLiteral(isInternal)),
                createObjectProperty("component", isInternal ? t.stringLiteral(elementName) : t.identifier(elementName)),
                createObjectProperty("props", t.objectExpression([])),  // Props would be processed fully in real implementation
                createObjectProperty("id", t.stringLiteral(generateShortId())),
                createObjectProperty("key", keyAttr && keyAttr.value
                    ? t.isJSXExpressionContainer(keyAttr.value)
                        ? keyAttr.value.expression as t.Expression
                        : t.stringLiteral((keyAttr.value as t.StringLiteral).value)
                    : indexParam
                )
            ]);
        }

        // If not JSX, pass through the returned expression
        return returnStmt.argument;
    }

    // Fallback if no return statement found
    return t.objectExpression([
        createObjectProperty("$$internalComponent", t.booleanLiteral(true)),
        createObjectProperty("component", t.stringLiteral("div")),
        createObjectProperty("props", t.objectExpression([])),
        createObjectProperty("id", t.stringLiteral(generateShortId())),
        createObjectProperty("key", indexParam)
    ]);
}

/**
 * Extract the tag name from a JSX opening element
 */
export function getJsxElementName(openingElement: t.JSXOpeningElement): string {
    const nameNode = openingElement.name;
    if (t.isJSXIdentifier(nameNode)) {
        return nameNode.name;
    } else if (t.isJSXMemberExpression(nameNode)) {
        // For JSX member expressions like Namespace.Component
        let fullName = '';
        let current: t.JSXMemberExpression | t.JSXIdentifier = nameNode;

        while (t.isJSXMemberExpression(current)) {
            fullName = '.' + current.property.name + fullName;
            current = current.object;
        }

        if (t.isJSXIdentifier(current)) {
            fullName = current.name + fullName;
        }

        return fullName;
    }

    throw new Error(`Unsupported JSX element name type: ${openingElement.name.type}`);
}
