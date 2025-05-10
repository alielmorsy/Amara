function createRef(initialValue, stateVariable = true) {
    const VALUE_KEY = Symbol('stateValue');
    const container = {
        [VALUE_KEY]: initialValue,
        // Store primitive type info for proper coercion
        get [Symbol.toPrimitive]() {
            return this[VALUE_KEY][Symbol.toPrimitive];
        }
    };

    const handler = {
        get(target, prop, receiver) {
            print("Get called")
            // Handle special properties
            if (prop === 'value' || prop === '_internalValueOf') return target[VALUE_KEY];
            if (prop === 'setValue') return (newValue) => {
                target[VALUE_KEY] = newValue
            };
            if (prop === '_isStateVariable') return stateVariable;

            // Handle value coercion
            if (prop === Symbol.toPrimitive) {
                const val = target[VALUE_KEY];
                return val[Symbol.toPrimitive] || function (hint) {
                    return hint === 'string' ? String(val) : Number(val);
                };
            }

            // Access actual value's properties
            const currentValue = target[VALUE_KEY];
            const valueObj = (typeof currentValue === 'object' && currentValue !== null)
                ? currentValue
                : Object(currentValue);

            const value = Reflect.get(valueObj, prop, receiver);
            return typeof value === 'function' ? value.bind(valueObj) : value;
        },

        set(target, prop, value, receiver) {
            // Forward property sets to the actual value
            if (typeof target[VALUE_KEY] === 'object' && target[VALUE_KEY] !== null) {
                return Reflect.set(target[VALUE_KEY], prop, value, receiver);
            }
            throw new TypeError("Cannot set property on primitive value");
        },

        apply(target, thisArg, argumentsList) {
            // Handle function calls
            if (typeof target[VALUE_KEY] === 'function') {
                return Reflect.apply(target[VALUE_KEY], thisArg, argumentsList);
            }
            throw new TypeError("Proxy value is not a function");
        },

        construct(target, argumentsList, newTarget) {
            // Handle new operator
            if (typeof target[VALUE_KEY] === 'function') {
                return Reflect.construct(target[VALUE_KEY], argumentsList, newTarget);
            }
            throw new TypeError("Proxy value is not a constructor");
        },

        // Add trap for Object.keys, Object.entries, etc.
        ownKeys(target) {
            const value = target[VALUE_KEY];
            if (typeof value === 'object' && value !== null) {
                return Reflect.ownKeys(value);
            }
            return [];
        },

        // Add trap for property descriptors
        getOwnPropertyDescriptor(target, prop) {
            const value = target[VALUE_KEY];
            if (typeof value === 'object' && value !== null) {
                return Reflect.getOwnPropertyDescriptor(value, prop);
            }
            return undefined;
        },

        // Add other necessary traps
        getPrototypeOf() {
            return Reflect.getPrototypeOf(container[VALUE_KEY]);
        },

        has(target, prop) {
            if (prop === "_isStateVariable") {
                return true;
            }
            return Reflect.has(container[VALUE_KEY], prop);
        },
        spread() {
            return {...container[VALUE_KEY]};
        },

        // Support for Object spread {...obj}
        defineProperty(target, prop, descriptor) {
            if (typeof target[VALUE_KEY] === 'object' && target[VALUE_KEY] !== null) {
                return Reflect.defineProperty(target[VALUE_KEY], prop, descriptor);
            }
            return false;
        }
    };

    return new Proxy(container, handler);
}

function toRaw(value) {
    return value._isStateVariable ? value.value : value
}

function diffAndUpdate(obj1, obj2) {
    const changes = {
        added: [],
        removed: [],
        changed: [],
    };

    // Move helper functions outside of main function for better performance
    function isObject(val) {
        return val && typeof val === 'object' && !Array.isArray(val);
    }

    function isEqual(a, b) {
        // Fast path for identity comparison
        if (a === b) return true;
        if (a == null || b == null) return false;
        if (typeof a !== typeof b) return false;

        // Object comparison
        if (isObject(a) && isObject(b)) {
            const aKeys = Object.keys(a);
            const bKeys = Object.keys(b);
            if (aKeys.length !== bKeys.length) return false;

            // Use a for loop instead of .every() for better performance
            for (let i = 0; i < aKeys.length; i++) {
                const key = aKeys[i];
                if (!b.hasOwnProperty(key) || !isEqual(a[key], b[key])) {
                    return false;
                }
            }
            return true;
        }

        // Array comparison
        if (Array.isArray(a) && Array.isArray(b)) {
            if (a.length !== b.length) return false;

            for (let i = 0; i < a.length; i++) {
                if (!isEqual(a[i], b[i])) return false;
            }
            return true;
        }

        return false;
    }

    function compareAndUpdate(o1, o2, path = '') {
        // Early return for edge cases
        if (o1 === o2) return;
        if (!o1 || !o2) {
            // Handle null/undefined cases
            if (!o1 && o2) {
                Object.assign(o1, o2);
                changes.added.push({key: path, value: o2});
            } else if (o1 && !o2) {
                for (const key in o1) {
                    delete o1[key];
                }
                changes.removed.push({key: path, value: o1});
            }
            return;
        }

        // Create key sets once
        const keys1 = Object.keys(o1);
        const keys2 = Object.keys(o2);
        const keys1Set = new Set(keys1);
        const keys2Set = new Set(keys2);

        // Handle added keys
        for (const key of keys2) {
            if (key === 'children') continue;
            //events will always be different, so we don't need to check on it.
            if (key.startsWith("on")) continue;

            const fullPath = path ? `${path}.${key}` : key;

            if (!keys1Set.has(key)) {
                o1[key] = o2[key];
                changes.added.push({key: fullPath, value: o2[key]});
            }
        }

        // Handle removed and changed keys
        for (const key of keys1) {
            if (key === 'children') continue;

            const fullPath = path ? `${path}.${key}` : key;
            const val1 = o1[key];

            if (!keys2Set.has(key)) {
                delete o1[key];
                changes.removed.push({key: fullPath, value: val1});
            } else {
                const val2 = o2[key];

                if (!isEqual(val1, val2)) {
                    if (isObject(val1) && isObject(val2)) {
                        compareAndUpdate(val1, val2, fullPath);
                    } else {
                        o1[key] = val2;
                        changes.changed.push({key: fullPath, from: val1, to: val2});
                    }
                }
            }
        }
    }

    compareAndUpdate(obj1, obj2);
    return changes;
}