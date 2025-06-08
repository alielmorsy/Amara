export interface Update<T = any> {
    lane: number;
    action: T | ((prev: T) => T);
    next: Update<T> | null;
}

export class UpdateQueue<T = any> {
    baseState: T;
    firstBaseUpdate: Update<T> | null = null;
    lastBaseUpdate: Update<T> | null = null;
    shared: {
        pending: Update<T> | null;
    } = {pending: null};

    constructor(baseState: T) {
        this.baseState = baseState;
    }

    enqueueUpdate(update: Update<T>): void {
        const pending = this.shared.pending;
        if (pending === null) {
            update.next = update;
        } else {
            update.next = pending.next;
            pending.next = update;
        }
        this.shared.pending = update;
    }
}
