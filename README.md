# Hopscotch Hashing Implementation in C

This project implements a hash table using the Hopscotch Hashing algorithm in C. This data structure provides efficient dictionary operations (insertion, lookup, and deletion) with O(1) constant time performance.

## What is Hopscotch Hashing?

Hopscotch Hashing is a hash table algorithm developed by Maurice Herlihy, Nir Shavit, and Moran Tzafrir in 2008. It combines the advantages of open and closed hashing, and is designed to perform well even with high load factors.

### Key features:

- **Neighborhood chaining**: Each key is stored in a position within its "neighborhood," which is a range of positions close to its original hash location.
- **Low-latency access**: Ensures that all keys are found near their ideal hash position.
- **High space utilization**: Works well even when the table is nearly full (load factor of 90%).
- **Predictable performance**: Avoids the extreme degradation that occurs in other hash implementations as the load factor increases.

## Data Structure

The implementation uses the following structures:

```c
typedef struct Element {
  const char *key;
  void *value;
  size_t hashvalue;
} Element;

typedef struct dictionary {
  Element *elements;     // Array of elements
  size_t size;           // Number of stored elements
  size_t capacity;       // Total capacity of the array
  destroy_f destroy;     // Function to free values
  uint32_t *bitmap;      // Bitmap to track occupied positions
} dictionary_t;
```

## Configuration Parameters

```c
#define INITIAL_CAPACITY 1024   // Initial table capacity
#define HOP_RANGE 32            // Maximum neighborhood size
#define BIT_RANGE 0xFFFFFFFF    // Bitmap mask
#define SEED 42                 // Seed for hash function
```

## Algorithm

### Main Functions

1. **Insertion (`dictionary_put`)**:
   - Calculates the key's hash
   - Checks if the key already exists
   - If there's space in the neighborhood, inserts directly
   - If not, displaces existing elements to make room
   - If necessary, performs rehashing to expand the table

2. **Lookup (`dictionary_get`)**:
   - Calculates the key's hash
   - Searches for the key in its neighborhood using the bitmap
   - Returns the associated value or NULL if not found

3. **Deletion (`dictionary_delete`, `dictionary_pop`)**:
   - Locates the key in its neighborhood
   - Removes the element and updates the bitmap
   - Returns the value or destroys it depending on the operation

### Collision Handling

Hopscotch Hashing's strategy for handling collisions includes:

1. **Finding free space within the neighborhood**: Each key must be stored within the neighborhood of its original hash position.

2. **Element displacement** (`displace_keys`): When a neighborhood is full, existing elements are rearranged to make room for the new key.

3. **Rehashing** (`rehash`): If rearranging elements isn't possible, the table is resized by doubling its capacity and all elements are redistributed.

### Optimizations

- **Bitmap usage**: Neighborhoods are managed using bitmaps, which reduces memory accesses.
- **Prefetching**: Use of `__builtin_prefetch` to improve cache performance.
- **Efficient bit operations**: Use of techniques like `__builtin_clz` for fast bitmap operations.
- **Murmur hash function**: Provides high-quality distribution with good avalanche properties.

## Dictionary API

```c
// Creates a new dictionary with an optional destruction function
dictionary_t *dictionary_create(destroy_f destroy);

// Inserts or updates a key-value pair
bool dictionary_put(dictionary_t *dictionary, const char *key, void *value);

// Gets a value based on its key
void *dictionary_get(dictionary_t *dictionary, const char *key, bool *err);

// Removes a key and its associated value
bool dictionary_delete(dictionary_t *dictionary, const char *key);

// Removes and returns a value without destroying it
void *dictionary_pop(dictionary_t *dictionary, const char *key, bool *err);

// Checks if a key exists
bool dictionary_contains(dictionary_t *dictionary, const char *key);

// Returns the number of elements
size_t dictionary_size(dictionary_t *dictionary);

// Frees the dictionary and its elements
void dictionary_destroy(dictionary_t *dictionary);
```

## Usage Examples

### Basic Example

```c
#include "hopscotch_hash.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Create a dictionary that frees values with free
    dictionary_t *dict = dictionary_create(free);
    
    // Insert some key-value pairs
    char *value1 = strdup("value1");
    char *value2 = strdup("value2");
    
    dictionary_put(dict, "key1", value1);
    dictionary_put(dict, "key2", value2);
    
    // Look up a value
    bool err;
    char *result = dictionary_get(dict, "key1", &err);
    if (!err) {
        printf("Value for key1: %s\n", result);
    }
    
    // Check existence
    if (dictionary_contains(dict, "key2")) {
        printf("key2 exists in the dictionary\n");
    }
    
    // Free the dictionary and all its values
    dictionary_destroy(dict);
    
    return 0;
}
```

## Performance

The Hopscotch Hashing implementation offers:

- O(1) operations under normal conditions
- High performance even with load factors of 90%
- Predictable scalability as the number of elements increases

Performance tests (`tests.c`) include:
- Tests with up to 1,048,576 elements
- Random insertion and deletion tests
- Coherence verification after many operations

## References

- Herlihy, M., Shavit, N., & Tzafrir, M. (2008). "Hopscotch Hashing". DISC '08: Proceedings of the 22nd international symposium on Distributed Computing.

## License

This code is distributed under the MIT license.
