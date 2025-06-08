import {currentFiber, Update, UpdateQueue, workLoop, WorkPriority} from "./tmp";

function useState<T>(initialValue: T): [T, (value: T | ((prev: T) => T)) => void] {
    if (!currentFiber) {
        throw new Error('useState must be called within a component');
    }

    const fiber = currentFiber;
    let hook = getCurrentHook();

    if (hook === null) {
        // Mount
        hook = {
            memoizedState: initialValue,
            baseState: initialValue,
            baseQueue: null,
            queue: new UpdateQueue(initialValue),
            next: null
        };

        fiber.memoizedState = hook;
    }

    const queue = hook.queue;
    const dispatch = (action: T | ((prev: T) => T)) => {
        const update: Update<T> = {
            lane: workLoop.pickArbitraryLane(WorkPriority.Normal),
            action,
            next: null
        };

        queue.enqueueUpdate(update);
        workLoop.scheduleWork(fiber, WorkPriority.Normal);
    };

    return [hook.memoizedState, dispatch];
}
