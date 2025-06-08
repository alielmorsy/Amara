import {ReactiveProxy} from "../tmp";
import {UpdateQueue} from "./UpdateQueue";
import {FiberEffectTag, FiberTag} from "./enums";
export class FiberNode {
    // Fiber identification
    tag: FiberTag;
    key: string | null = null;
    elementType: any = null;
    type: any = null;

    // Fiber tree structure
    return: FiberNode | null = null;      // Parent
    child: FiberNode | null = null;       // First child
    sibling: FiberNode | null = null;     // Next sibling
    index: number = 0;

    // Work-in-progress tree
    alternate: FiberNode | null = null;

    // Props and state
    pendingProps: any = null;
    memoizedProps: any = null;
    memoizedState: any = null;
    updateQueue: UpdateQueue | null = null;

    // Effects
    effectTag: FiberEffectTag = FiberEffectTag.NoEffect;
    nextEffect: FiberNode | null = null;
    firstEffect: FiberNode | null = null;
    lastEffect: FiberNode | null = null;

    // Work scheduling
    lanes: number = 0;           // Priority lanes
    pendingLanes: number = 0;           // Priority lanes
    childLanes: number = 0;

    // Component-specific data
    stateNode: any = null;       // DOM node or component instance
    dependencies: Set<ReactiveProxy> = new Set();
    isStatic: boolean = false;   // Our optimization flag

    constructor(tag: FiberTag, pendingProps: any, key: string | null) {
        this.tag = tag;
        this.pendingProps = pendingProps;
        this.key = key;
    }
}

export function createFiberRoot(): FiberNode {
    const root = new FiberNode(FiberTag.HostRoot, null, null);
    return root;
}
