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

let c = createRef({})
c._internalValueOf = true;
let d = [1, c]

function _slicedToArray(r, e) {
    return _arrayWithHoles(r) || _iterableToArrayLimit(r, e) || _unsupportedIterableToArray(r, e) || _nonIterableRest();
}

function _nonIterableRest() {
    throw new TypeError("Invalid attempt to destructure non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method.");
}

function _unsupportedIterableToArray(r, a) {
    if (r) {
        if ("string" == typeof r) return _arrayLikeToArray(r, a);
        var t = {}.toString.call(r).slice(8, -1);
        return "Object" === t && r.constructor && (t = r.constructor.name), "Map" === t || "Set" === t ? Array.from(r) : "Arguments" === t || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(t) ? _arrayLikeToArray(r, a) : void 0;
    }
}

function _arrayLikeToArray(r, a) {
    (null == a || a > r.length) && (a = r.length);
    for (var e = 0, n = Array(a); e < a; e++) n[e] = r[e];
    return n;
}
function toRaw(value) {
    return value._isStateVariable ? value.value : value
}
function _iterableToArrayLimit(r, l) {
    var t = null == r ? null : "undefined" != typeof Symbol && r[Symbol.iterator] || r["@@iterator"];
    if (null != t) {
        var e, n, i, u, a = [], f = !0, o = !1;
        try {
            if (i = (t = t.call(r)).next, 0 === l) {
                if (Object(t) !== t) return;
                f = !1;
            } else for (; !(f = (e = i.call(t)).done) && (a.push(e.value), a.length !== l); f = !0) ;
        } catch (r) {
            o = !0, n = r;
        } finally {
            try {
                if (!f && null != t.return && (u = t.return(), Object(u) !== u)) return;
            } finally {
                if (o) throw n;
            }
        }
        return a;
    }
}

function _arrayWithHoles(r) {
    if (Array.isArray(r)) return r;
}

function TaskBoard() {
    beginComponentInit("guaLtoio");

    var _useState = useState({
            todo: ['Buy milk', 'Write blog'],
            doing: ['Learn React'],
            done: []
        }),
        _useState2 = _slicedToArray(_useState, 2),
        columns = _useState2[0],
        setColumns = _useState2[1];
    var _useState3 = useState(''),
        _useState4 = _slicedToArray(_useState3, 2),
        newTask = _useState4[0],
        setNewTask = _useState4[1];
    var handleAddTask = () => {
        if (!newTask.trim()) return;

        setColumns(prev => {
            print("Items", [...prev.todo, toRaw(newTask)])
            return ({
                ...prev,
                todo: [...prev.todo, toRaw(newTask)]
            })
        });
        setNewTask('');
    };
    var moveTask = (task, from, to) => {
        setColumns(prev => {
            const newFrom = prev[from].filter(t => t !== task);
            const newTo = [...prev[to], task];

            return {
                ...prev,
                [from]: newFrom,
                [to]: newTo
            };
        });
    };
    effect(() => {
        setNewTask("3lawy")

    }, [])
    effect(() => {
        handleAddTask()
    }, [newTask])
    {
        var _parent = createElement("div", {
            style: {
                display: 'flex',
                gap: '1rem'
            }
        });
        var _mapParent = createElement("component", {});
        effect(() => {
            print("Column Keys", columns['todo'])
            listConciliar(_mapParent, Object.keys(columns), (col, _index) => ({
                "$$internalComponent": true,
                "component": "div",
                "props": {
                    key: col,
                    style: {
                        border: '1px solid #ccc',
                        padding: '1rem',
                        width: '200px'
                    },
                    "children": [{
                        "$$internalComponent": true,
                        "component": "text",
                        "props": {
                            "children": [col.toUpperCase()]
                        },
                        "id": "PGhPdRNm"
                    }, ...columns[col].map(task => ({
                        "$$internalComponent": true,
                        "component": "div",
                        "props": {
                            key: task,
                            style: {
                                marginBottom: '0.5rem'
                            },
                            "children": [{
                                "$$internalComponent": true,
                                "component": "text",
                                "props": {
                                    "children": [task]
                                },
                                "id": "LYnNSJCJ"
                            }]
                        },
                        "id": "1twSLwZf"
                    }))]
                },
                "id": "eyfQ7mK3"
            }));
        }, [columns]);
        _parent.addChild(_mapParent);
        var _element4 = createElement("div", {});
        effect(() => {
            _element4.insertChild("cbRvNlP3", {
                "$$internalComponent": true,
                "component": "component",
                "props": {
                    value: newTask,
                    onChange: e => setNewTask(e.target.value),
                    "children": []
                },
                "id": "mk7KGhg7"
            });
        }, []);
        _element4.addStaticChild({
            "$$internalComponent": true,
            "component": "button",
            "props": {
                onClick: handleAddTask,
                "children": []
            },
            "id": "wdn7e03"
        });
        _parent.addChild(_element4);
        endComponent();
        return _parent;
    }
}

render(TaskBoard)