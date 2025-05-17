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
    beginComponentInit("sVQwNuCG");
    const [columns, setColumns] = useState({
        todo: ['Buy milk', 'Write blog'],
        doing: ['Learn React'],
        done: []
    });
    const [newTask, setNewTask] = useState('');
    const handleAddTask = () => {
        //@amara-state
        const example = columns;
        if (!newTask.trim()) return;
        setColumns(prev => ({
            ...prev,
            todo: [...prev.todo, toRaw(newTask)]
        }));
        columns.todo.push(toRaw(example));
        setNewTask('');
    };
    const moveTask = (task, from, to) => {
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
        const _parent = createElement("div", {
            style: {
                display: 'flex',
                gap: '1rem'
            }
        });
        const _mapParent = createElement("component", {});
        effect(() => {
            listConciliar(_mapParent, Object.keys(toRaw(columns)), (col, _index) => ({
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
                        "component": 'text',
                        "props": {
                            "children": [col.toUpperCase()]
                        },
                        "id": "BVRijwT7"
                    }, ...columns[col].map(task => ({
                        "$$internalComponent": false,
                        "component": Child,
                        "props": {
                            task: task,
                            key: col,
                            col: col,
                            "children": [
                                {
                                    "$$internalComponent": true,
                                    "component": 'text',
                                    "props": {
                                        "children": ["MYYY"]
                                    },
                                    "id": "BVRijwT7"
                                }
                            ]
                        },
                        "id": "j2NTT4PU"
                    }))]
                },
                "id": "tv3Dmam"
            }));
        }, [columns]);
        _parent.addChild(_mapParent);
        const _element2 = createElement("div", {});
        effect(() => {
            _element2.insertChild("Os9p4w13", {
                "$$internalComponent": true,
                "component": "div",
                "props": {
                    value: toRaw(newTask),
                    onChange: e => setNewTask(e.target.value),
                    "children": []
                },
                "id": "LGJB0j4O"
            });
        }, [newTask]);
        _element2.addStaticChild({
            "$$internalComponent": true,
            "component": "text",
            "props": {
                onClick: handleAddTask,
                "children": ["Add Task"]
            },
            "id": "xF9628E"
        });
        _parent.addChild(_element2);
        endComponent();
        return _parent;
    }
}


function Child({children,
                   task,
                   col,
                   moveTask
               }) {
    beginComponentInit("KEOKV9N8");
    {
        const _parent4 = createElement("div", {
            key: toRaw(task),
            style: {
                marginBottom: '0.5rem'
            }
        });
        effect(() => {
            _parent4.key = task;
        }, [task]);
        const _element6 = createElement("text", {});
        effect(() => {
            _element6.insertChild("lPjJ8JRt", toRaw(task));
        }, [task]);
        _parent4.addChild(_element6);
        const _element7 = createElement("div", {});
        _element7.addStaticChild({
            "$$internalComponent": true,
            "component": "text",
            "props": {
                "children": ["Move To"]
            },
            "id": "5IAYCAQ6"
        });
        const _holder = createElement("holder", {});
        effect(() => {
            _holder.setChild({
                "$$internalComponent": true,
                "component": "text",
                "props": {
                    onClick: () => moveTask(toRaw(task), toRaw(col), 'todo'),
                    "children": ["To Do"]
                },
                "id": "P567npIC"
            });
        }, [moveTask]);
        effect(() => {
            if (col !== 'todo') {
                _element7.insertChild("lRJuCVsR", _holder);
            } else {
                _element7.removeChild("lRJuCVsR");
            }
        }, [col]);
        const _holder2 = createElement("holder", {});
        effect(() => {
            _holder2.setChild({
                "$$internalComponent": true,
                "component": "text",
                "props": {
                    onClick: () => moveTask(toRaw(task), toRaw(col), 'doing'),
                    "children": ["Doing"]
                },
                "id": "ENfwYMaT"
            });
        }, [moveTask]);
        effect(() => {
            if (col !== 'doing') {
                _element7.insertChild("GGN60Egd", _holder2);
            } else {
                _element7.removeChild("GGN60Egd");
            }
        }, [col]);
        const _holder3 = createElement("holder", {});
        effect(() => {
            _holder3.setChild({
                "$$internalComponent": true,
                "component": "text",
                "props": {
                    onClick: () => moveTask(toRaw(task), toRaw(col), 'done'),
                    "children": ["Done"]
                },
                "id": "yLCTEA3c"
            });
        }, [moveTask]);
        effect(() => {
            if (col !== 'done') {
                _element7.insertChild("870Ncen", _holder3);
            } else {
                _element7.removeChild("870Ncen");
            }
        }, [col]);
        _parent4.addChild(_element7);
        print("Children",children)
        _parent4.insertChildren(children)
        endComponent();
        return _parent4;
    }
}

render(TaskBoard)