import { defineConfig } from 'tsup';

export default defineConfig([
    // Main transform package
    {
        entry: ['src/index.ts'],
        format: ['cjs', 'esm'],
        dts: true,
        sourcemap: true,
        clean: true,
        minify: true,
        external: ['@babel/core', '@babel/types', '@babel/traverse'],
    },

]);
