import {ComponentChild} from "./tmp";

export class AmaraElement {
    public children: (AmaraElement | ComponentChild | string)[] = [];
    public properties: Record<string, any> = {};
    public id?: string;
    private childrenById: Map<string, AmaraElement | ComponentChild> = new Map();

    constructor(
        public tag: string,
        props: Record<string, any> = {},
        id?: string
    ) {
        this.properties = { ...props };
        this.id = id;
    }


    addChild(child: AmaraElement | string): void {
        this.children.push(child);
    }

    addStaticChild(child: ComponentChild): void {
        this.children.push(child);
        this.childrenById.set(child.id, child);
    }

    insertChild(id: string, child: ComponentChild | AmaraElement | string): void {
        const existingIndex = this.children.findIndex(c =>
            typeof c === 'object' && 'id' in c && c.id === id
        );

        if (existingIndex !== -1) {
            this.children[existingIndex] = child;
        } else {
            this.children.push(child);
        }

        if (typeof child === 'object' && 'id' in child) {
            this.childrenById.set(id, child);
        }
    }

    insertChildren(children: ComponentChild[]): void {
        const holder = new AmaraElement('holder');
        children.forEach(child => holder.addStaticChild(child));
        this.addChild(holder);
    }

    setChild(child: ComponentChild): void {
        this.children = [child];
        this.childrenById.set(child.id, child);
    }

    // Style manipulation for direct updates
    get style(): Record<string, any> {
        return this.properties.style || {};
    }

    set style(newStyle: Record<string, any>) {
        this.properties.style = { ...this.properties.style, ...newStyle };
    }
}
