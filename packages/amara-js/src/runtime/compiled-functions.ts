import {AmaraElement} from "../AmaraElement";

export function createElement(tag: string, props: Record<string, any> = {}): AmaraElement {
    return new AmaraElement(tag, props);
}

export function createRef<T>(initialValue: T, stateVariable = true):T {
    const VALUE_KEY = Symbol('stateValue');

    const container: { [VALUE_KEY]: T } = {
        [VALUE_KEY]: initialValue
    };

    const handler: ProxyHandler<typeof container> = {
        get(target, prop, receiver) {
            // Custom log function — replace with console.log or remove
            console.log("Get called");

            if (prop === 'value' || prop === '_internalValueOf') {
                return target[VALUE_KEY];
            }

            if (prop === 'setValue') {
                return (newValue: T) => {
                    target[VALUE_KEY] = newValue;
                };
            }

            if (prop === '_isStateVariable') {
                return stateVariable;
            }

            if (prop === Symbol.toPrimitive) {
                const val = target[VALUE_KEY];
                return typeof val === 'object' && val !== null && Symbol.toPrimitive in val
                    ? (val as any)[Symbol.toPrimitive]
                    : (hint: string) => (hint === 'string' ? String(val) : Number(val));
            }

            const currentValue = target[VALUE_KEY];
            const valueObj =
                typeof currentValue === 'object' && currentValue !== null
                    ? currentValue
                    : Object(currentValue);

            const value = Reflect.get(valueObj, prop, receiver);
            return typeof value === 'function' ? value.bind(valueObj) : value;
        },

        set(target, prop, value, receiver) {
            if (typeof target[VALUE_KEY] === 'object' && target[VALUE_KEY] !== null) {
                return Reflect.set(target[VALUE_KEY] as any, prop, value, receiver);
            }
            throw new TypeError("Cannot set property on primitive value");
        },

        apply(target, thisArg, argumentsList) {
            if (typeof target[VALUE_KEY] === 'function') {
                return Reflect.apply(target[VALUE_KEY] as any, thisArg, argumentsList);
            }
            throw new TypeError("Proxy value is not a function");
        },

        construct(target, argumentsList, newTarget) {
            if (typeof target[VALUE_KEY] === 'function') {
                return Reflect.construct(target[VALUE_KEY] as any, argumentsList, newTarget);
            }
            throw new TypeError("Proxy value is not a constructor");
        },

        ownKeys(target) {
            const value = target[VALUE_KEY];
            if (typeof value === 'object' && value !== null) {
                return Reflect.ownKeys(value);
            }
            return [];
        },

        getOwnPropertyDescriptor(target, prop) {
            const value = target[VALUE_KEY];
            if (typeof value === 'object' && value !== null) {
                return Reflect.getOwnPropertyDescriptor(value, prop);
            }
            return undefined;
        },

        getPrototypeOf() {
            return Reflect.getPrototypeOf(container[VALUE_KEY] as never);
        },

        has(target, prop) {
            if (prop === "_isStateVariable") {
                return true;
            }
            return Reflect.has(container[VALUE_KEY] as any, prop);
        },

        defineProperty(target, prop, descriptor) {
            const value = target[VALUE_KEY];
            if (typeof value === 'object' && value !== null) {
                return Reflect.defineProperty(value, prop, descriptor);
            }
            return false;
        }
    };



    return new Proxy(container, handler) as T;
}
