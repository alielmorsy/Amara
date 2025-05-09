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