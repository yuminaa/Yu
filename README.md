# Yu Programming Language

[![Build Status](https://github.com/yuminaa/yu/actions/workflows/workflow.yml/badge.svg)](https://github.com/yuminaa/yu/actions)
[![codecov](https://codecov.io/gh/yuminaa/yu/branch/main/graph/badge.svg)](https://codecov.io/gh/yuminaa/yu)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Yu is a statically typed, compiled programming language designed for data-oriented programming while maintaining familiar OOP-like syntax. It emphasizes data layout, cache coherency, and efficient memory patterns while providing a comfortable, modern programming interface.

> [!IMPORTANT] 
> The compiler is currently under development and not ready for production use.

## Features

- **Data-Oriented Design**: First-class support for structured data layouts and cache-friendly patterns
- **Static Typing**: Strong type system with generic support
- **Memory Layout Control**: Attributes like `@packed` and `@aligned` for precise memory layout control
- **Memory Safety**: Reference counting/smart pointers for automatic memory management
- **Familiar Syntax**: OOP-like syntax for better readability while maintaining data-oriented principles
- **Performance-Oriented**: Optimized compilation with multiple optimization levels
- **Generic Programming**: Full support for generic types and functions
- **Module System**: Simple and intuitive import system

## Philosophy

Yu uses class-like syntax as a familiar interface for defining data structures and their associated operations, but internally treats data and behavior separately. This approach:

- Prioritizes data layout and memory patterns
- Encourages thinking about data transformation rather than object behavior
- Maintains cache coherency through deliberate data organization
- Provides precise control over memory layout with attributes

## Code Example

```yu
import { io } from 'system'; 

@packed
@aligned(16)
class Vector3
{
    var x: i32 = 0;
    var y: i32 = 0;
    var z: i32 = 0;
};

class Player 
{
    private var position: Vector3 = Vector3({ x = 10, y = 20, z = -10 });
    private var velocity: Vector3 = Vector3();

    public static inline dot(vec1: Vector3, vec2: Vector3) -> f32 
    {
        return (vec1.x * vec2.x) + (vec1.y * vec2.y) + (vec1.z * vec2.z);
    }
};

// Generic data container
class Generic<T>
{
    var value: T;
    public inline function GetValue() -> T 
    {
        return self.value;
    }
};
```

## Data-Oriented Features

### Memory Layout Control
```yu
// Explicit control over struct packing
@packed
class PackedData 
{
    var a: i32 = 0;
    var b: i16 = 0;
}

// Memory alignment for SIMD operations
@aligned(16)
class SimdFriendly 
{
    var data: [ f32 ];
}
```

# Roadmap

- [X] Lexer
- [ ] Parser
- [ ] Semantic Analysis
- [ ] Code Generation
- [ ] Optimization
- [ ] Standard Library