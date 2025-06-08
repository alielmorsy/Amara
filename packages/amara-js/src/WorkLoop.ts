import {FiberEffectTag, FiberNode, FiberTag, WorkPriority} from "./fibers";
import {AmaraElement} from "./AmaraElement";

class WorkLoop {
    private workInProgressRoot: FiberNode | null = null;
    private workInProgress: FiberNode | null = null;
    private currentTime: number = 0;
    private workInProgressRootRenderLanes: number = 0;
    private rootWithPendingPassiveEffects: FiberNode | null = null;

    // Time slicing
    private shouldYield = false;
    private yieldInterval = 5; // 5ms time slice
    private deadline = 0;
    private workLoopRoot: null = null;

    scheduleWork(fiber: FiberNode, priority: WorkPriority): void {
        const root = this.markUpdateLaneFromFiberToRoot(fiber, priority);
        if (root !== null) {
            this.ensureRootIsScheduled(root);
        }
    }

    private markUpdateLaneFromFiberToRoot(fiber: FiberNode, priority: WorkPriority): FiberNode | null {
        // Traverse up to find root
        let node = fiber;
        let parent = fiber.return;

        while (parent !== null) {
            node = parent;
            parent = parent.return;
        }

        // Mark lanes for priority
        const lane = this.pickArbitraryLane(priority);
        fiber.lanes |= lane;

        // Bubble up child lanes
        let alternate = fiber.alternate;
        if (alternate !== null) {
            alternate.lanes |= lane;
        }

        this.markUpdateLaneFromFiberToRoot(fiber.return!, priority);

        return node.tag === FiberTag.HostRoot ? node : null;
    }

    private pickArbitraryLane(priority: WorkPriority): number {
        switch (priority) {
            case WorkPriority.Immediate:
                return 1;
            case WorkPriority.UserBlocking:
                return 2;
            case WorkPriority.Normal:
                return 4;
            case WorkPriority.Low:
                return 8;
            case WorkPriority.Idle:
                return 16;
            default:
                return 4;
        }
    }

    private ensureRootIsScheduled(root: FiberNode): void {
        // Schedule work based on priority
        this.performSyncWorkOnRoot(root);
    }

    private performSyncWorkOnRoot(root: FiberNode): void {
        const lanes = this.getNextLanes(root);
        if (lanes === 0) return;

        this.prepareFreshStack(root, lanes);

        do {
            try {
                this.workLoopSync();
                break;
            } catch (thrownValue) {
                this.handleError(root, thrownValue);
            }
        } while (true);

        this.finishSyncRender(root);
    }

    private workLoopSync(): void {
        while (this.workInProgress !== null) {
            this.performUnitOfWork(this.workInProgress);
        }
    }

    private workLoopConcurrent(): void {
        while (this.workInProgress !== null && !this.shouldYieldToHost()) {
            this.performUnitOfWork(this.workInProgress);
        }
    }

    private shouldYieldToHost(): boolean {
        const timeElapsed = performance.now() - this.deadline;
        return timeElapsed >= this.yieldInterval;
    }

    private performUnitOfWork(unitOfWork: FiberNode): void {
        const current = unitOfWork.alternate;
        let next = this.beginWork(current, unitOfWork, this.workInProgressRootRenderLanes);

        unitOfWork.memoizedProps = unitOfWork.pendingProps;

        if (next === null) {
            this.completeUnitOfWork(unitOfWork);
        } else {
            this.workInProgress = next;
        }
    }

    private beginWork(current: FiberNode | null, workInProgress: FiberNode, renderLanes: number): FiberNode | null {
        // Clear lanes being worked on
        workInProgress.lanes = 0;

        switch (workInProgress.tag) {
            case FiberTag.FunctionComponent:
                return this.updateFunctionComponent(current, workInProgress, renderLanes);
            case FiberTag.HostComponent:
                return this.updateHostComponent(current, workInProgress);
            case FiberTag.HostText:
                return null;
            case FiberTag.StaticComponent:
                return this.updateStaticComponent(current, workInProgress);
            default:
                return null;
        }
    }

    private updateFunctionComponent(current: FiberNode | null, workInProgress: FiberNode, renderLanes: number): FiberNode | null {
        const Component = workInProgress.type;
        const props = workInProgress.pendingProps;

        // Set current fiber for hooks
        currentFiber = workInProgress;

        const children = Component(props);

        currentFiber = null;

        this.reconcileChildren(current, workInProgress, children);
        return workInProgress.child;
    }

    private updateStaticComponent(current: FiberNode | null, workInProgress: FiberNode): FiberNode | null {
        // Static components skip reconciliation if not dirty
        if (current !== null && !workInProgress.dependencies.size) {
            // Clone the current tree
            workInProgress.child = current.child;
            return workInProgress.child;
        }

        // Otherwise treat as normal function component
        return this.updateFunctionComponent(current, workInProgress, this.workInProgressRootRenderLanes);
    }

    private updateHostComponent(current: FiberNode | null, workInProgress: FiberNode): FiberNode | null {
        const type = workInProgress.type;
        const nextProps = workInProgress.pendingProps;
        const prevProps = current !== null ? current.memoizedProps : null;

        let nextChildren = nextProps.children;

        this.reconcileChildren(current, workInProgress, nextChildren);
        return workInProgress.child;
    }

    private reconcileChildren(current: FiberNode | null, workInProgress: FiberNode, nextChildren: any): void {
        if (current === null) {
            workInProgress.child = this.mountChildFibers(workInProgress, null, nextChildren, this.workInProgressRootRenderLanes);
        } else {
            workInProgress.child = this.reconcileChildFibers(workInProgress, current.child, nextChildren, this.workInProgressRootRenderLanes);
        }
    }

    private mountChildFibers(returnFiber: FiberNode, currentFirstChild: FiberNode | null, newChild: any, lanes: number): FiberNode | null {
        // Simplified child mounting
        if (typeof newChild === 'object' && newChild !== null) {
            if (Array.isArray(newChild)) {
                return this.reconcileChildrenArray(returnFiber, currentFirstChild, newChild, lanes);
            } else {
                return this.reconcileSingleElement(returnFiber, currentFirstChild, newChild, lanes);
            }
        }
        return null;
    }

    private reconcileChildFibers(returnFiber: FiberNode, currentFirstChild: FiberNode | null, newChild: any, lanes: number): FiberNode | null {
        return this.mountChildFibers(returnFiber, currentFirstChild, newChild, lanes);
    }

    private reconcileSingleElement(returnFiber: FiberNode, currentFirstChild: FiberNode | null, element: any, lanes: number): FiberNode | null {
        const created = this.createFiberFromElement(element, lanes);
        created.return = returnFiber;
        return created;
    }

    private reconcileChildrenArray(returnFiber: FiberNode, currentFirstChild: FiberNode | null, newChildren: any[], lanes: number): FiberNode | null {
        let resultingFirstChild: FiberNode | null = null;
        let previousNewFiber: FiberNode | null = null;

        for (let i = 0; i < newChildren.length; i++) {
            const newFiber = this.createChild(returnFiber, newChildren[i], lanes);
            if (newFiber === null) continue;

            if (previousNewFiber === null) {
                resultingFirstChild = newFiber;
            } else {
                previousNewFiber.sibling = newFiber;
            }
            previousNewFiber = newFiber;
        }

        return resultingFirstChild;
    }

    private createChild(returnFiber: FiberNode, newChild: any, lanes: number): FiberNode | null {
        if (typeof newChild === 'object' && newChild !== null) {
            const created = this.createFiberFromElement(newChild, lanes);
            created.return = returnFiber;
            return created;
        }
        return null;
    }

    private createFiberFromElement(element: any, lanes: number): FiberNode {
        const type = element.type || element.component;
        const key = element.key;
        const pendingProps = element.props || {};

        let fiberTag: FiberTag;
        if (typeof type === 'string') {
            fiberTag = FiberTag.HostComponent;
        } else if (typeof type === 'function') {
            fiberTag = element.$$internalComponent === false ? FiberTag.FunctionComponent : FiberTag.StaticComponent;
        } else {
            fiberTag = FiberTag.Fragment;
        }

        const fiber = new FiberNode(fiberTag, pendingProps, key);
        fiber.elementType = element;
        fiber.type = type;
        fiber.lanes = lanes;

        return fiber;
    }

    private completeUnitOfWork(unitOfWork: FiberNode): void {
        let completedWork: FiberNode | null = unitOfWork;

        do {
            const current = completedWork.alternate;
            const returnFiber = completedWork.return;

            if ((completedWork.effectTag & FiberEffectTag.DidCapture) === FiberEffectTag.NoEffect) {
                let next = this.completeWork(current, completedWork, this.workInProgressRootRenderLanes);

                if (next !== null) {
                    this.workInProgress = next;
                    return;
                }

                if (returnFiber !== null && (returnFiber.effectTag & FiberEffectTag.DidCapture) === FiberEffectTag.NoEffect) {
                    if (returnFiber.firstEffect === null) {
                        returnFiber.firstEffect = completedWork.firstEffect;
                    }
                    if (completedWork.lastEffect !== null) {
                        if (returnFiber.lastEffect !== null) {
                            returnFiber.lastEffect.nextEffect = completedWork.firstEffect;
                        }
                        returnFiber.lastEffect = completedWork.lastEffect;
                    }

                    if (completedWork.effectTag > FiberEffectTag.NoEffect) {
                        if (returnFiber.lastEffect !== null) {
                            returnFiber.lastEffect.nextEffect = completedWork;
                        } else {
                            returnFiber.firstEffect = completedWork;
                        }
                        returnFiber.lastEffect = completedWork;
                    }
                }
            }

            const siblingFiber = completedWork.sibling;
            if (siblingFiber !== null) {
                this.workInProgress = siblingFiber;
                return;
            }

            completedWork = returnFiber;
            this.workInProgress = completedWork;
        } while (completedWork !== null);
    }

    private completeWork(current: FiberNode | null, workInProgress: FiberNode, renderLanes: number): FiberNode | null {
        switch (workInProgress.tag) {
            case FiberTag.HostComponent:
                if (current !== null && workInProgress.stateNode != null) {
                    // Update existing node
                    this.markUpdate(workInProgress);
                } else {
                    // Create new node
                    const instance = this.createInstance(workInProgress);
                    workInProgress.stateNode = instance;
                    if (workInProgress.child !== null) {
                        this.appendAllChildren(instance, workInProgress);
                    }
                }
                return null;

            case FiberTag.HostText:
                if (current !== null && workInProgress.stateNode != null) {
                    this.markUpdate(workInProgress);
                } else {
                    workInProgress.stateNode = workInProgress.pendingProps;
                }
                return null;

            default:
                return null;
        }
    }

    private createInstance(fiber: FiberNode): AmaraElement {
        return new AmaraElement(fiber.type, fiber.pendingProps);
    }

    private appendAllChildren(parent: AmaraElement, workInProgress: FiberNode): void {
        let node = workInProgress.child;

        while (node !== null) {
            if (node.tag === FiberTag.HostComponent || node.tag === FiberTag.HostText) {
                parent.addChild(node.stateNode);
            } else if (node.child !== null) {
                node.child.return = node;
                node = node.child;
                continue;
            }

            if (node === workInProgress) {
                return;
            }

            while (node.sibling === null) {
                if (node.return === null || node.return === workInProgress) {
                    return;
                }
                node = node.return;
            }

            node.sibling.return = node.return;
            node = node.sibling;
        }
    }

    private markUpdate(workInProgress: FiberNode): void {
        workInProgress.effectTag |= FiberEffectTag.Update;
    }

    private prepareFreshStack(root: FiberNode, lanes: number): void {
        this.workInProgressRoot = root;
        this.workInProgress = this.createWorkInProgress(root, null);
        this.workInProgressRootRenderLanes = lanes;
    }

    private createWorkInProgress(current: FiberNode, pendingProps: any): FiberNode {
        let workInProgress = current.alternate;

        if (workInProgress === null) {
            workInProgress = new FiberNode(current.tag, pendingProps, current.key);
            workInProgress.elementType = current.elementType;
            workInProgress.type = current.type;
            workInProgress.stateNode = current.stateNode;

            workInProgress.alternate = current;
            current.alternate = workInProgress;
        } else {
            workInProgress.pendingProps = pendingProps;
            workInProgress.effectTag = FiberEffectTag.NoEffect;
            workInProgress.nextEffect = null;
            workInProgress.firstEffect = null;
            workInProgress.lastEffect = null;
        }

        workInProgress.childLanes = current.childLanes;
        workInProgress.lanes = current.lanes;
        workInProgress.child = current.child;
        workInProgress.memoizedProps = current.memoizedProps;
        workInProgress.memoizedState = current.memoizedState;
        workInProgress.updateQueue = current.updateQueue;

        return workInProgress;
    }

    private finishSyncRender(root: FiberNode): void {
        this.workLoopRoot = null;
        this.commitRoot(root);
    }

    private commitRoot(root: FiberNode): void {
        const finishedWork = root;

        // Commit effects
        if (finishedWork.firstEffect !== null) {
            this.commitBeforeMutationEffects(finishedWork.firstEffect);
            this.commitMutationEffects(finishedWork.firstEffect, root);
            this.commitLayoutEffects(finishedWork.firstEffect, root);
        }
    }

    private commitBeforeMutationEffects(firstEffect: FiberNode): void {
        // Handle before mutation effects
    }

    private commitMutationEffects(firstEffect: FiberNode, root: FiberNode): void {
        let nextEffect: FiberNode | null = firstEffect;

        while (nextEffect !== null) {
            const effectTag = nextEffect.effectTag;

            if (effectTag & FiberEffectTag.Placement) {
                this.commitPlacement(nextEffect);
            }

            if (effectTag & FiberEffectTag.Update) {
                this.commitWork(nextEffect.alternate, nextEffect);
            }

            if (effectTag & FiberEffectTag.Deletion) {
                this.commitDeletion(nextEffect, root);
            }

            nextEffect = nextEffect.nextEffect;
        }
    }

    private commitLayoutEffects(firstEffect: FiberNode, root: FiberNode): void {
        // Handle layout effects
    }

    private commitPlacement(finishedWork: FiberNode): void {
        // Handle DOM placement
    }

    private commitWork(current: FiberNode | null, finishedWork: FiberNode): void {
        // Handle DOM updates
    }

    private commitDeletion(current: FiberNode, root: FiberNode): void {
        // Handle DOM deletion
    }

    private getNextLanes(root: FiberNode): number {
        // Return pending lanes
        return root.pendingLanes || 0;
    }

    private handleError(root: FiberNode, thrownValue: any): void {
        // Error handling
        console.error('Fiber work error:', thrownValue);
    }
}
