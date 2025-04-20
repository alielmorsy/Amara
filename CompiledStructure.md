# Compiler Structure

Each component (Function that returns a JSX element) must have `beginComponentInit` at the beginning of the function and
`endComponent` at right before the return.

Let's use this example

```jsx
function Component() {

    return <div>
        <span>One</span>
        <span>Two</span>
    </div>
}
```

> Note: I used HTML tags to make it easier to understand yet. They won't exist in the actual implementation

This function is marked as a component. Now let's talk about how React would handle that:

- It will call the component function to create the VDOM which contains the props, tag name, and key if existed.
- It will compare the tree with the previous tree, and for if the changes are minimal, the algorithm shall edit the
  actual dom nodes directly rather than recreating.

I tried to take a different approach. First, a babel plugin will convert that component to something different like
this:

```js
function Component() {
    beginComponentInit();
    const parent = createElment("div", {});
    parent.addStaticChild({
        $$internalComponent: true,
        component: "span",
        props: {
            children: ["One"]
        },
        id: "MTJgvvRR"
    });

    parent.addStaticChild({
        $$internalComponent: true,
        component: "span",
        props: {
            children: ["Two"]
        },
        id: "dPwEOeiD"
    });
    return parent;
}
```

What have I made different? I marked components as static, meaning they will never need to be recalled or re-rendered
again; In fact we don't need any diffing to check that! That sound
weird, right? Let's see another example:

```jsx
function ParentComponent() {
...
    return <div>
        <ChildComponent/>
    </div>
}

function ChildComponent() {
    return <div>
        <span>One</span>
        <span>Two</span>
    </div>
}
```

In React, if for any reason ParentComponent was re-rendered, a Child component vdom tree will be recreated and added to
the vdom tree, and the diff tree will eventually check if it was updated or not. The babel plugin will do something a
bit crazier. See how the Parent Component will look like:

```js
function ParentComponent() {
    beginComponentInit(); // A must (Will be added automatically)
...//Any other code
    const parent = createElment("div", {});
    parent.addStaticChild({
        $$internalComponent: false,
        component: ChildComponent,
        props: {},
        id: "randomIDKey1"
    });
}
```

See the magic here? If a Parent component was recalled a million time ChildComponent will never be recalled, How? On the
initial call (The first call for the algorithm) the `ChildComponent` will be rendered without any checks. But for any
reason that the component had to be recalled, the ChildComponent will be moved automatically to the new vdom to reduce
any calculations.

### How would we handle the children of a static component?

For this issue, we have two cases:

1. All children are static so we can pass them directly. Like this:

```js
parent.addStaticChild({
    $$internalComponent: false,
    component: ChildComponent,
    props: {
        children: [
            {
                $$internalComponent: true,
                component: "text",
                props: {
                    children: ["Hello from Child One"]
                },
                id: "CHILDIDONE"
            },
            {
                $$internalComponent: true,
                component: "text",
                props: {
                    children: ["Hello from Child Two."]
                },
                id: "CHILDIDONE"
            }
        ]
    },
    id: "randomIDKey1"
});
```

2. One or more of the children depend on a variable. This one is a bit tricky for so many reasons because we may have
   children that will never change. So I am between two solutions:
    1. Define these functions in the parent component as elements (Call them directly) but if the parent changed we may
       have unnecessary calculations.
    2. Put them as objects and render them down there. in specific parent. Something like that:

```js
function Test({children, name}) {
    beginComponentInit();
    const [counter, setCounter] = useState(4)
    const [d, setD] = useState(4)
    const _parent5 = createElement("component", {});
    //insert children directly into the parent 5
    _parent5.insertChildren(children)
    effect(() => {
        const _element5 = {
            $$internalComponent: true,
            component: "text",
            props: {
                children: [
                    name + counter
                ]
            },
            id: "zUblYAq"
        }

        _parent5.insertChild("zUblYAq", _element5);
    }, [name, counter])
    effect(() => {
        setCounter(7)
    }, [])
    endComponent();
    return _parent5;
}
```

Under the hood, `insertChildren` will create a parent that holds all these children to make it easier to reconcile. The
reason is kinda weird, but it makes total sense. To make diffing as lower as possible.

> You may notice that unlike `insertChild` `insertChildren` doesn't take ID. The reason is kinda simple. children will
> always be an array, each element in these children will always have an ID, so we can insert items directly
<hr>
This theory may look simple and assuming that ChildComponent has no states. That's true, and that's why we introduce the second game changer feature (Inspired by vue). Let's modify the ChildComponent a bit:

```js
function ChildComponent() {
    const [name, setName] = useState("Ali");
    return <div>
        <span>hello {name}</span>
        <span>Two</span>
    </div>
}
```

What is happening here? I am defining a simple state and use it one of the children. What also we notice here that the
second child won't be re-rendered ever again. Let's head back to the first child. It has a state variable in it, So,
that's a dynamic child, How would we solve this issue, If we marked this component as never rendered? The answer comes
in the concept of reactivity. Let's see how the compiler will output that:

```js
function ChildComponent() {
    beginComponentInit();
    const [name, setName] = useState("Ali");
    const parent = createElment("div", {});
    const spanOne = createElement("span", {});
    spanOne.addChild("hello");
    effect(() => {
        spaneOne.insertChild("zUblYAq", name);
    }, [name]);
    parent.addChild(spanOne);
    parent.addStaticChild({
        $$internalComponent: true,
        component: "span",
        props: {
            children: ["Two"]
        },
        id: "dPwEOeiD"
    });
    return parent;

}
```

In this example, we did three different things:

1. The first span is acting as a parent, meaning we called `addChild` on it by invoking `parent.addChild(spanOne)` to add that span to the tree.

2. We introduced a new internal function called `effect`.

3. We also introduced `insertChild`.

- The logic behind `insertChild` is straightforward: we add the child to a specific location in the tree, then store the index of that location.
- The reason behind the first note is to ensure that, if the `effect` function is called, we know exactly where to insert the child.
- All IDs are generated at compile time to reduce runtime overhead, so most operations are O(1).

> **IMPORTANT NOTE**: The `effect` function may look similar to the `useEffect` hook, but they are completely different.

> All components are transformed in a way that ensures they can function either as static or dynamic components. This guarantees a robust implementation across different use cases of the same component.

## The magic of the effect function

In the `ChildComponent` above, the first `<span>` contains a child element that depends on a piece of state. To handle
this dependency, we introduced the use of an `effect`. When the state changes, the entire component is marked as "dirty"
—meaning it's flagged for an update—but it’s not re-rendered immediately. Instead, it’s placed in a queue waiting to be
reprocessed. During the next render cycle, each item in the queue is updated one by one. In this update phase, we
directly update the specific state variable that changed and then trigger any associated effects that rely on that
state.

## How does the backend mutate the variable directly?

At first glance, this might seem strange—how can we mutate a variable directly in JavaScript? The answer lies in a
powerful feature: the Proxy object.

### How proxy works?

A `Proxy` is a special JavaScript class that lets you wrap a value while still interacting with it like a normal
variable. Take a look at this example:

```js
const ref = createProxy(5);
console.log(ref + 5) //output: 10
ref.setValue({name: "Ali", age: 24}) //Not exposed it just an example.
console.log(ref.name)//Output: Ali
console.log(ref.age)//Output: 24

ref.major = "CS";
console.log(ref.major)//Output: CS
```

See what’s happening? `createProxy` returns a variable that behaves just like a regular one, but with a twist—it can be
updated behind the scenes. This is our secret sauce for updating internal component state without re-rendering the
entire component. Instead, we only trigger the effects associated with what actually changed.

## Avoid re-rendering on primitive prop changes

Let’s look at a real-world example:

```jsx
function ChildComponent() {
    const [width, setWidth] = useState(0)
    useEffect(() => {
        setTimeout(() => {
            setWidth(150)
        }, 1000)
    }, []);
    return <div style={{
        width: `${width}px`,
        height: '100px'
    }}>
        <span>
          Hi!
          </span>
    </div>
}
```

Here, the only change happening is to the width of the `<div>`. Re-rendering the whole component (and triggering a full
diff) just for that? Seems a bit overkill.

### So, What’s the Smarter Way?

We let the effect update only what’s changed—nothing more, nothing less. Here's what the compiled output might look
like:

```js
function ChildComponent() {
    beginComponentInit();
    const [width, setWidth] = useState(0)
    useEffect(() => {
        setTimeout(() => {
            setWidth(150)
        }, 1000)
    }, []);

    const parent = createElment("div", {
        style: {
            width: `${width}px`,
            height: '100px'
        }
    });
    const spanOne = createElement("span", {});
    spanOne.addChild("Hi!");
    effect(() => {
        parent.style.width = `${width}px`;
    }, [width]);
    endComponent();
    return parent;
}
```

🚀 Voilà! We’ve updated the width directly—no need to recreate the component, no unnecessary diffs, and no wasted
computation.

Another thing can be noticed, What if the changed prop doesn't belong to primitive. It's a component prop.

To address this issue. Let's understand the issue first. Consider this example

```jsx
function ParentComponent() {
    const [name, setName] = useState("Ali")
    return <div>
        <ChildComponent name={name}/>
    </div>
}

function ChildComponent({name}) {
    return <div>
        <span>Hi {name}</span>
        <span>Static span "will never change"</span>
    </div>
}
```

This example is basically a refiner to the first example. The only difference is now our ChildComponent has a prop.
Unfortunately, now we need a smart way to reduce diffing as possible. Handling Parent Component can be easy. Try to
figure it yourself:

```js

function ParentComponent() {
    beginComponentInit();
    const [name, setName] = useState("Ali")
    const parent = createElement("div", {});
    effect(() => {
        parent.insertChild("someRandomComponentID", {
            $$internalComponent: true,
            component: ChildComponent,
            props: {
                name: name
            },
            id: "someRandomComponentID"
        });
    }, [name]);
    endComponent();
    return parent;
}
```

Got It? It's okay if not, It may be hard at first. Let explain it. Now we know that ChildComponent has to be recalled
because it has a state that may be changed from time to time. So, we used `insertChild` here again. But here it's a bit
different.

It does the same thing, meaning it gets the old widget based on the ID but instead of replacing the value directly we
create a sub virtual tree and compare it with the current one. These compares won't include any static children as we
marked them as static before.

> Maybe the term static can be changed to constant IDK.

> **IMPORTANT NOTE** Maybe I didn't explain it above, but effect function will be called immediately on the first
> component call. Because as you can see it contains adding children, so it must be called in the same order.
<hr>
## Array Mapping

One of the most known patterns in React is array mapping. Something like that

```jsx
function Component() {
    const [arr, setArr] = useState([]);
    useEffect(() => {
        //Simulate network request
        setArr(["Item One", "Item Two"])
    }, []);

    return <div>
        {arr.map((val, index) => (
            <span key={index}>{val}</span>
        ))}
    </div>
}
```

Most found solutions were slow, except a solution was discovered by accident. We can do the mapping internally and
reconcile and diff the way we want.

We introduce another internal function called `listConcile` which work pretty much same as `arr.map` except its not. See
The output of the compiler:

```js
function Component() {
    beginComponentInit();
    const [arr, setArr] = useState([]);
    useEffect(() => {
        //Simulate network request
        setArr(["Item One", "Item Two"])
    }, []);

    const parent = createElement("div", {});

    const mapHolder = createElement("holder", {});
    parent.addChild(mapHolder);
    effect(() => {
        listConcile(mapHolder, arr, (value, index) => {
            return {
                $$internalComponent: true,
                component: "span",
                props: {
                    children: [value]
                },
                key: index
            }
        });
    }, [arr])

}
```

What was done here is pretty interesting. We add a new child to work as an immediate way between the parent widget and
the list reconcile function.

### How `listConcile` works under the hood.

It's a simple function that loops through the given array, and for each element it calls the given widget function and
diff both subtrees while forcing the same constrains mentioned before.

## Why `beginComponentInit` and `endComponent`?

Each component (Function that returns a JSX) will have states, effects, useEffect, useMemo,...etc. or any other internal
hook. All these data need to be stored somewhere accessible, and that place needs to be associated with these widgets to
avoid overlapping. So, the backend has a stack when `beginComponentInit` is called a new component instance is created
and added to the stack, and any hook call, will be associated with that component. When the component is done (right
before the return statement) `endComponent` will pop up that component from the stack, and that instance will live
inside the widgets of these components.

## The current big issue which I have no solution for it

I have no ducking way how to implement contexts. Contexts were pretty much the black magic in Context which every.

## Limitations and how to solve it:

As you can tell moving states between components, can make produce some unneeded rendering. To avoid this, this
framework must provide a global state which can pretty much enhance the performance by managing the global state by the
engine. 
