import {processFunc} from "./transform/FunctionTransform";
import type * as BabelCore from "@babel/core";



const Plugin = (_babel: typeof BabelCore): BabelCore.PluginObj => {
    return {
        name: "amara-jsx-transform",
        visitor: {
            Program(path, state) {
                console.log("Handling File:", state.filename)
            },
            FunctionDeclaration: (path, state) => {

                processFunc(path)
            },
            FunctionExpression: (path, state) => {
                processFunc(path)
            },
            ArrowFunctionExpression: (path, state) => {
                processFunc(path)
            },


        },
        //Adding jsx to the parsing flow
        manipulateOptions(opts, parserOpts) {
            parserOpts.plugins.push("jsx");
        }
    }
}


export default Plugin;
