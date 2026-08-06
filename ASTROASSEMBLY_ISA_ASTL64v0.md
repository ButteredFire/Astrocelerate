# Astrocelerate Assembly Language (AstroAssembly)
## Instruction Set Architecture, Version ASTL64v0
Author: Dương Duy Nhật Minh (Minh Dương | ButteredFire)

---

## 1. Description
AstroAssembly ("As"-Two Language, `.astl`) is a bytecode language designed for Astrocelerate's visual scripting architecture. 

Simulations are mostly programmed via the visual graph, existing on disk as a YAML configuration file. While initial simulation state configuration is directly read from YAML, the visual graph is compiled to AstroAssembly and executed by a dedicated Virtual Machine.

---

## 2. Definitions, Conventions & Policies

### I. The Virtual Machine
The AstroAssembly Bytecode Virtual Machine (VM) is a Big-Endian, hybrid-architecture software VM that uses a LIFO stack data structure as its primary mechanism for storing operands, as well as special-purpose registers (SPRs) to store metadata.

* Maximum Stack Size: 10 KB default (Configurable up to 1 MB)
* Maximum Call Stack Size: 10 KB default (Configurable up to 1 MB)
* Word Size: 64 bits
* SPR Size: 64 bits
* Instruction Size: Fixed-length, 32-bit (4 bytes) instructions.
* Endianness: Big-Endian (Byte 0 is the Most Significant Byte, Byte 7 is the Least Significant Byte).

### II. Containers

#### 2.1. The Stack
The VM stack is a LIFO container storing 64-bit slots of data. It is a buffer that stores transient data for calculations and comparisons. Every instruction that reads from the VM stack consumes (pops) what it has read.

Values well-defined in size are stored directly on the stack. Arbitrarily sized values (e.g., strings) are stored on the stack as 64-bit indices pointing into other containers:
* Bytes 0-1: Container type
* Bytes 2-6: Unused bits
* Bytes 7-8: Value's index into the container

#### 2.2. Special-Purpose Registers (SPRs)
Each SPR stores 64 bits of data pertaining to VM state, execution flow state, and special simulation states.

* `<0x00>` VMS (Virtual Machine States):
  + Bytes 0-1: VM exit code
  + Bytes 2-3: Current program counter
  + Bytes 4-5: Current stack size (KB)
  + Byte 6: Instruction bitmask
  + Byte 7: Debugger bitmask
* `<0x01>` EXC: Stores the current name of the execution pin that triggered the current basic block.
  + Bytes 0-8: Container type holding the string, and index into that container.

#### 2.3. Variable Registries
Data containers for persistent, reusable data tied to a specific registry scope.
* Global Registry: Globally accessible; valid for the entire simulation lifetime.
* Local Registries: Live on the frame stack; scoped to the allocating procedure. Using local-registry instructions outside procedures is illegal.

#### 2.4. Constant Pools
A table of structures representing literals and symbolic references, resolved at compile time.

#### 2.5. Container Types
* `<0x00>` VM Stack
* `<0x01>` SPRs
* `<0x02>` Global Registry
* `<0x03>` Local Registry
* `<0x04>` Constant Pool
*(Implementations may internally use other non-standard container type codes.)*

---

## 3. Data Types & Type Casting

### Natively Supported Types
* `<0x00>` BYTE (8-bit unsigned integer) - *Internal use only*
* `<0x01>` IDX (16-bit unsigned integer) - *Internal use only*
* `<0x02>` I16 (16-bit signed integer)
* `<0x03>` I32 (32-bit signed integer)
* `<0x04>` F64 (64-bit float)
* `<0x05>` BOOL (1-byte boolean)
* `<0x06>` VEC3 (24-byte 3-element F64 vector)
* `<0x07>` STR (Arbitrarily sized string)

Terminology:
* "Number": Refers to `I16`, `I32`, `F64`.
* "Address", "Index", "ID": Refers to `IDX`.
* "Slot T1" / "Slot T2": Type of the top / second-to-top VM stack slot.

### Type Casting
Convertible/mutually castable types can be cast to each other.
* Convertible types (transitively): `I16` <=> `I32` <=> `F64` <=> `BOOL`
* Widening casts apply automatically in arithmetic/logical operations between mutually castable types.

---

## 4. Execution & Error Behaviors

### Arithmetic and Logical Instructions
* Evaluated in FIFO order (e.g., stack `[3.14, 42]` with `SUB` equals `3.14 - 42`).
* VEC3: `MUL` and `DIV` support scalar broadcasts with a number. `NEG` acts as scalar multiplication by -1. All other arithmetic/logical operations involving `VEC3` or `BOOL` are illegal.
* IDX: Only `ADD` and `SUB` are valid, requiring both operands to be `IDX`.

### Exit Codes (Termination)
* `0`: `SUCCESS` *(Execution flow reached an explicit TERMINATE instruction)*
* `-1`: `CRASHED_CONFIG` *(Generic VM error that occurs at configuration time)*
* `-2`: `CRASHED_RT` *(Generic VM error that occurs at runtime)*
* `-3`: `EXEC_HALTED` *(Execution flow failed to reach an explicit TERMINATE instruction)*
* `-4`: `VM_STACK_OVERFLOW` *(The VM stack exceeded its maximum allocated size)*
* `-5`: `CALL_STACK_OVERFLOW` *(The call stack exceeded its maximum allocated size)*
* `-6`: `BAD_CAST` *(A value was casted to or reinterpreted as an incompatible type, or sourced from an incompatible origin)*
* `-7`: `OUT_OF_BOUNDS` *(A container was accessed with an out-of-bounds index)*

Positive exit codes are implementation-defined.

---

## 5. Instruction Set Reference

All instructions have a fixed length of 4 bytes. 
* Byte 0: Opcode (1 byte).
* Byte 1: (Usually) Bitmask (1 byte).
  + *Bit 0: Read-only flag (0 = consume on read, 1 = preserve on read).*

| Byte 0 (Opcode) | Byte 1 | Byte 2 | Byte 3 | Description |
| :--- | :--- | :--- | :--- | :--- |
| `LOAD_INLINE` | bitmask | number (MSB) | number (LSB) | Loads a small 16-bit signed number onto the VM stack |
| `POP` | bitmask | number (MSB) | number (LSB) | Pops an arbitrary number of elements from the VM stack |
| `LOAD_SPR` | bitmask | padding | Slot T1 | Loads a value (as a specific type) from a special-purpose register onto the VM stack |
| `STORE_SPR` | bitmask | padding | SPR Type | Stores the top VM stack slot into a special-purpose register (of a specific type/ID) |
| `LOAD_CONST` | bitmask | index (MSB) | index (LSB) | Loads a value (as a specific type) from the constant pool onto the VM stack |
| `LOAD_GL` | bitmask | index (MSB) | index (LSB) | Loads a value from the global registry (by index) onto the VM stack |
| `LOAD_LC` | bitmask | index (MSB) | index (LSB) | Loads a value from the local registry (by index) onto the VM stack |
| `STORE_GL` | bitmask | padding | Slot T1 | Stores the top VM stack slot (as a specific type) into global registry |
| `STORE_LC` | bitmask | padding | Slot T1 | Stores the top VM stack slot (as a specific type) into local registry |
| `OVR_GL` | bitmask | index (MSB) | index (LSB) | Overwrites a value from the global registry (by index) with that of the top VM stack slot |
| `OVR_LC` | bitmask | index (MSB) | index (LSB) | Overwrites a value from the local registry (by index) with that of the top VM stack slot |
| `TO_STR` | bitmask | padding | Slot T1 | Casts the top VM stack slot (of a specific type) to a STR stack slot |
| `TO_I16` | bitmask | padding | Slot T1 | Casts the top VM stack slot (of a specific type) to an I16 stack slot |
| `TO_I32` | bitmask | padding | Slot T1 | Casts the top VM stack slot (of a specific type) to an I32 stack slot |
| `TO_F64` | bitmask | padding | Slot T1 | Casts the top VM stack slot (of a specific type) to an F64 stack slot |
| `PRINT` | bitmask | padding | Slot T1 | Prints the value of the top VM stack slot (of a specific type) to the current output stream |
| `EXEC_NATIVE` | bitmask | index (MSB) | index (LSB) | Executes a native function by its function index |
| `ADD` | bitmask | Slot T2 | Slot T1 | Evaluates (A + B) |
| `SUB` | bitmask | Slot T2 | Slot T1 | Evaluates (A - B) |
| `MUL` | bitmask | Slot T2 | Slot T1 | Evaluates (A * B) |
| `DIV` | bitmask | Slot T2 | Slot T1 | Evaluates (A / B) |
| `MOD` | bitmask | Slot T2 | Slot T1 | Evaluates (A % B) |
| `NEG` | bitmask | Slot T2 | Slot T1 | Evaluates (-A) |
| `STR_CAT` | bitmask | padding | padding | Concatenates the top two STR VM stack slots and pushes the result onto the stack |
| `VEC_NORM` | bitmask | padding | padding | Vector normalization |
| `VEC_MAG` | bitmask | padding | padding | Vector magnitude |
| `VEC_MUL` | bitmask | padding | padding | Vector component-wise multiplication (Equivalent to MUL with two VEC3 slots) |
| `VEC_DIV` | bitmask | padding | padding | Vector component-wise division (Equivalent to DIV with two VEC3 slots) |
| `VEC_MUL_DOT` | bitmask | padding | padding | Vector dot product |
| `VEC_MUL_CROSS`| bitmask | padding | padding | Vector cross product |
| `CMP_LT` | bitmask | Slot T2 | Slot T1 | Evaluates (A < B) |
| `CMP_LTE` | bitmask | Slot T2 | Slot T1 | Evaluates (A <= B) |
| `CMP_GT` | bitmask | Slot T2 | Slot T1 | Evaluates (A > B) |
| `CMP_GTE` | bitmask | Slot T2 | Slot T1 | Evaluates (A >= B) |
| `CMP_EQ` | bitmask | Slot T2 | Slot T1 | Evaluates (A == B) |
| `CMP_NEQ` | bitmask | Slot T2 | Slot T1 | Evaluates (A != B) |
| `LGC_AND` | bitmask | Slot T2 | Slot T1 | Evaluates (A && B) |
| `LGC_OR` | bitmask | Slot T2 | Slot T1 | Evaluates (A \|\| B) |
| `LGC_NOT` | bitmask | padding | padding | Evaluates (!A) |
| `SIN` | bitmask | padding | padding | Computes Sine |
| `ASIN` | bitmask | padding | padding | Computes Arcsine |
| `COS` | bitmask | padding | padding | Computes Cosine |
| `ACOS` | bitmask | padding | padding | Computes Arccosine |
| `TAN` | bitmask | padding | padding | Computes Tangent |
| `ATAN` | bitmask | padding | padding | Computes Arctangent |
| `ATAN2` | bitmask | padding | padding | Computes `atan2(y, x)`, where the VM stack is structured as `[y, x]` |
| `COT` | bitmask | padding | padding | Computes Cotangent |
| `ACOT` | bitmask | padding | padding | Computes Arccotangent |
| `ABS` | bitmask | padding | padding | Computes Absolute value |
| `MIN` | bitmask | padding | padding | Computes Minimum of two values |
| `MAX` | bitmask | padding | padding | Computes Maximum of two values |
| `JUMP` | bitmask | address (MSB) | address (LSB) | Moves the instruction pointer to another index in the bytecode array |
| `JUMP_IF_TRUE` | bitmask | address (MSB) | address (LSB) | Moves the instruction pointer if the topmost boolean is true |
| `JUMP_IF_FALSE`| bitmask | address (MSB) | address (LSB) | Moves the instruction pointer if the topmost boolean is false |
| `CALL` | bitmask | address (MSB) | address (LSB) | Calls a subroutine (pushes return address to call stack) |
| `CALL_IF_TRUE` | bitmask | address (MSB) | address (LSB) | Calls a subroutine if the topmost boolean is true |
| `CALL_IF_FALSE`| bitmask | address (MSB) | address (LSB) | Calls a subroutine if the topmost boolean is false |
| `RET` | bitmask | bitmask | bitmask | Pops return address from call stack and jumps back |
| `TERMINATE` | bitmask | exit code (MSB) | exit code (LSB) | Terminates the simulation with an inline exit code (I16 number) |