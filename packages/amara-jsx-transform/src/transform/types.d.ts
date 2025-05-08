import {NodePath, types as t} from "@babel/core";

export type  BabelFunction =
    | NodePath<t.FunctionDeclaration>
    | NodePath<t.FunctionExpression>
    | NodePath<t.ArrowFunctionExpression>;

export type JSXPath = NodePath<t.JSXElement>

export interface FunctionScope {
    variables: Set<string>;
    specialFlags: Record<string, boolean>
}

export type LiteralType = string | number | boolean
