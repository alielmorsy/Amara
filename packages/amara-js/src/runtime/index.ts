/**
 * Core runtime implementation for Amara JS
 * This file contains platform-agnostic runtime code that's shared between web and native
 */

import { 
  Component, 
  ComponentType, 
  ReactElement, 
  ReactNode,
  ReactContext,
  Ref,
  MutableRefObject,
  EffectCallback,
  DependencyList,
  Dispatcher,
  useState,
  useEffect,
  useContext,
  useRef,
  useCallback,
  useMemo
} from './types';

// Re-export all types
export * from './types';

// Current component instance being rendered
let currentComponentInstance: any = null;
let currentHookIndex = 0;

// Context for tracking the current dispatcher
let currentDispatcher: Dispatcher | null = null;

// Create a context
function createContext<T>(defaultValue: T, displayName?: string): ReactContext<T> {
  const context: ReactContext<T> = {
    Provider: ({ value, children }) => {
      // Implementation for Provider
      return children as any;
    },
    Consumer: ({ children }) => {
      // Implementation for Consumer
      return children(defaultValue);
    },
  };
  
  if (displayName) {
    context.displayName = displayName;
  }
  
  return context;
}

// Create element function (JSX factory)
function createElement(
  type: string | ComponentType,
  config: Record<string, any> | null = {},
  ...children: ReactNode[]
): ReactElement {
  const props: Record<string, any> = { ...config };
  
  // Handle children
  if (children.length === 1) {
    props.children = children[0];
  } else if (children.length > 1) {
    props.children = children;
  }
  
  // Handle refs and keys
  const ref = props.ref;
  const key = props.key;
  
  // Filter out special props
  if (props.hasOwnProperty('ref')) delete props.ref;
  if (props.hasOwnProperty('key')) delete props.key;
  
  return {
    type,
    props,
    key: key ?? null,
    ref,
  };
}

// Fragment component
const Fragment = ({ children }: { children?: ReactNode }): ReactNode => {
  return children;
};

// Forward ref
function forwardRef<T, P = {}>(
  render: (props: P, ref: Ref<T>) => ReactElement | null
): (props: P & { ref?: Ref<T> }) => ReactElement | null {
  return (props: P & { ref?: Ref<T> }) => {
    const { ref, ...rest } = props as { ref?: Ref<T>; [key: string]: any };
    return render(rest as P, ref || null);
  };
}

// Memo component
function memo<T extends ComponentType<any>>(
  Component: T,
  propsAreEqual?: (prevProps: any, nextProps: any) => boolean
): T {
  // In a real implementation, this would do shallow comparison of props
  // and only re-render if they change
  const MemoizedComponent = (props: any) => {
    return createElement(Component, props);
  };
  
  // Copy static properties from Component to MemoizedComponent
  return Object.assign(MemoizedComponent, Component) as T;
}

// Lazy loading
function lazy<T extends ComponentType<any>>(
  factory: () => Promise<{ default: T }>
): T {
  let Component: T | null = null;
  let promise: Promise<void> | null = null;
  
  return ((props: any) => {
    const [loaded, setLoaded] = useState(false);
    
    useEffect(() => {
      if (!promise) {
        promise = factory().then(module => {
          Component = module.default;
          setLoaded(true);
        });
      }
    }, []);
    
    if (!loaded || !Component) {
      // Return a placeholder or loading state
      return null;
    }
    
    return createElement(Component, props);
  }) as any;
}

// Suspense component
function Suspense({ children, fallback }: { children: ReactNode, fallback: ReactNode }): ReactNode {
  // In a real implementation, this would track promises and show fallback
  // while waiting for them to resolve
  return children;
}

// Export everything
export {
  createElement,
  createContext,
  useState,
  useEffect,
  useContext,
  useRef,
  useCallback,
  useMemo,
  forwardRef,
  memo,
  lazy,
  Suspense,
  Fragment,
};

export default {
  createElement,
  createContext,
  useState,
  useEffect,
  useContext,
  useRef,
  useCallback,
  useMemo,
  forwardRef,
  memo,
  lazy,
  Suspense,
  Fragment,
};