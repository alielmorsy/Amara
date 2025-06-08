import {AmaraElement} from "./AmaraElement";

interface ComponentProps {
    [key: string]: any;
    children?: ComponentChild[];
}


export interface EffectDependency {
    dependencies: any[];
    callback: () => void;
    id: string;
}
interface ComponentChild {
    $$internalComponent: boolean;
    component: string | ComponentFunction;
    props: ComponentProps;
    id: string;
    key?: string | number;
}

type ComponentFunction = (props?: ComponentProps) => AmaraElement;

export interface ComponentInstance {
    id: string;
    effects: EffectDependency[];
    states: Map<string, any>;
    isDirty: boolean;
    element?: AmaraElement;
}
