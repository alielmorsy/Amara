export enum FiberTag {
    FunctionComponent = 0,
    ClassComponent = 1,
    HostRoot = 2,
    HostComponent = 3,
    HostText = 4,
    Fragment = 5,
    StaticComponent = 6,
    ListComponent = 7,
}

export enum FiberEffectTag {
    NoEffect = 0,
    Placement = 1,
    Update = 2,
    Deletion = 4,
    ContentReset = 8,
    Callback = 16,
    DidCapture = 32,
    Ref = 64,
    Snapshot = 128,
}

export enum WorkPriority {
    Immediate = 1,
    UserBlocking = 2,
    Normal = 3,
    Low = 4,
    Idle = 5,
}
