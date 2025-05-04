import {processFunc} from "./transform/FunctionTransform";
import type * as BabelCore from "@babel/core";

const Plugin = (_babel: typeof BabelCore): BabelCore.PluginObj => {
    return {
        name: "amara-jsx-transform",
        visitor: {
            FunctionDeclaration: (path,state) => {
                processFunc(path)
            },
            FunctionExpression: processFunc,
            ArrowFunctionExpression: processFunc,

        },
        //Adding jsx to the parsing flow
        manipulateOptions(opts, parserOpts) {
            parserOpts.plugins.push("jsx");
        }
    }
}


export default Plugin;
