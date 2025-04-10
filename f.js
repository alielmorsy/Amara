/**
 Three types of adding child:
 - Static Child: Using the widget holder object to avoid recreating them.
 - Normal Adding for internal objects that may be change one of their props during runtime (Usually the style of class name in the future)
 - Inserting Children: Used only for components that one of their parameters change with state
 - We need to find a way to cache static children inside inserted component, That will pretty much enhance the performance x2

 */
function useMemo(fn) {
    return fn()
}


function createRef(initialValue) {
    const ref = function (...args) {
        if (typeof ref._value === "function") {
            return ref._value(...args);
        }
        throw new TypeError("Ref is not callable");
    };

    const dataType = typeof initialValue; // Store the initial type

    Object.assign(ref, {
        _value: initialValue,

        get value() {
            return this._value;
        },

        set value(newValue) {
            if (typeof newValue !== dataType) {
                throw new TypeError(`Value type must remain ${dataType}`);
            }

            this._value = newValue;
            if (dataType === "object" && newValue !== null) {
                Object.assign(this, newValue);
            }
        },
        _internalValueOf() {
            return ref._value
        },
        _isStateVariable() {
            return true;
        },
        [Symbol.toPrimitive](hint) {
            return typeof ref._value === "object" ? NaN : ref._value;
        },
        toString() {
            return ref._value.toString()
        }
    });

    if (dataType === "object" && initialValue !== null) {
        Object.assign(ref, initialValue);
    }

    return ref;
}


function ParentComponent() {
    beginComponentInit();
    const [state, setState] = useState({
        numbers: [1, 2, 3, 4]
    });
    {

        const _parent2 = createElement("component", {
            style: {
                color: "#ff0",
                display: "flex"
            }
        });

        const _element = createElement("text", {});
        _element.addText("Parent Component");
        _parent2.addChild(_element);

        effect(() => {
            const _element2 = {
                $$internalComponent: false,
                component: ChildComponent,
                props: ({
                    numbers: state.numbers,
                    children: []
                }),

            };
            _parent2.insertChild("MTJgvvRR", _element2);
        }, [state]);

        const _element3 = createElement('button', {
            onClick: () => setState("numbers", n => [...n, n.length + 1])
        });
        const _element3Text = createElement("text", {});
        _element3Text.addText("                Add Number ");
        _element3.addChild(_element3Text)
        effect(() => {
            _element3Text.insertChild("dPwEOeiD", state);
            //print(state.value.numbers)
        }, [state]);

        effect(() => {
            // print("Called empty value")
        }, [state])
        _parent2.addChild(_element3);

        endComponent();
        return _parent2;
    }
}

function ChildComponent({
                            numbers, children
                        }) {
    beginComponentInit();
    const doubledNumbers = numbers
    {
        const _parent4 = createElement("component", {});
        _parent4.addStaticChild({
            $$internalComponent: true,
            component: "text",
            props: {
                children: [
                    "Child Component"
                ]
            },
            id: "myDearID"
        });
        effect(() => {
            const _element5 = {
                $$internalComponent: false,
                component: GrandChildComponent,
                props: {
                    data: doubledNumbers
                },
                id: "zUblYAq"
            }
            _parent4.insertChild("zUblYAq", _element5);
        }, [doubledNumbers]);
        endComponent();
        return _parent4;
    }
}

function GrandChildComponent({
                                 data
                             }) {
    beginComponentInit();
    const [names, setNames] = useState(["ALi"]);
    {
        const _parent5 = createElement("component", {});
        _parent5.addStaticChild({
            $$internalComponent: true,
            component: "text",
            props: {
                children: [
                    "GrandChild Component"
                ]
            },
            id: "dearchildID"
        });

        const _element7 = createElement("text", {});
        //createParentBase
        const _parentBase = createElement("component", {});
        effect(() => {
            listConciliar(_parentBase, names, (value, index) => {

                return {
                    props: {
                        children: [
                            value
                        ]
                    },
                    $$internalComponent: true,
                    component: "text",
                    key: index, //That would make accessing it easier.


                }
            });

        }, [names]);
        setNames(["Ali", "Emad", "Hassan"])
        _parent5.addChild(_element7);
        _parent5.addChild(_parentBase);
        endComponent();
        return _parent5;
    }
}

render(ParentComponent)

//gc();
shutdown()