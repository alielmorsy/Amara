import { defineConfig } from 'tsup';

export default defineConfig([
    // Main CLI library
    {
        entry: ['src/index.ts'],
        format: ['cjs', 'esm'],
        dts: true,
        sourcemap: true,
        clean: true,
        minify: true,
        external: ['commander', 'chalk', 'ora', 'prompts', 'fs-extra'],
    },
    // CLI executable
    {
        entry: ['src/cli.ts'],
        format: ['cjs'],
        dts: false,
        sourcemap: false,
        minify: true,
        banner: {
            js: '#!/usr/bin/env node',
        },
        external: ['commander', 'chalk', 'ora', 'prompts', 'fs-extra'],
    },
]);
