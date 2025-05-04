import typescript from '@rollup/plugin-typescript';
import resolve from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import json from '@rollup/plugin-json';
import pkg from './package.json';

export default {
    input: 'src/index.ts', // Entry point for your plugin
    external: [
        ...Object.keys(pkg.devDependencies || {})
    ],
    output: {
        file: 'dist/index.js', // Output file
        format: 'cjs', // CommonJS format (compatible with Node.js and Babel)
        sourcemap: true, // Generate source maps for debugging
        exports: 'named',
    },
    plugins: [
        typescript({tsconfig: './tsconfig.json'}), // Compile TypeScript
        resolve(), // Resolve third-party modules
        commonjs(), // Convert CommonJS modules to ES modules
        json(), // Convert CommonJS modules to ES modules

    ],
};
