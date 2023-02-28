# IOExt
Extensions to IO handling in C

# IOExt.h
The API currently exposes 2 styles of iterating through a file by lines.  
`LineIterator`  
&emsp;struct that handles parsing an open file line by line, one at a time until `EOF` is encountered.  
`FileLineIterator`  
&emsp;struct that opens and parses a file line by line, one at a time until `EOF` is encountered. This iterator also handles clean-up of the underlying file.  
  
When you only want to iterator through the lines in one go, use `FileLineIterator`. For all other cases where special handling of the file is required, use `LineIterator`.

# iterators.h
This header file includes macros and variables for use with the `*Iterator` functionality in other parts of the library. Implicitly defines the requirements for an object to be `Iterable`.  
`enum iterator_status`  
&emsp;enums to control flow of the `Iterator`s.  
`for_each(ElementType, Element, IterableType, IterableInstance)`  
&emsp;macro allows for sequential iteration through an `Iterable` object  
&emsp;`IterableType` is the underlying object type that the macro will iterate over. For `Iterator`s that do not have a global struct defined of such a type, simply remove `Iterator` from the name, e.g. to use a `FileLineIterator`, `IterableType` should be `FileLine`.  
&emsp;`IteratbleInstance` is a pointer to an `IterableType`.  
&emsp;`Element` is the name to be associated with an element of the `IterableInstance`.  
&emsp;`ElementType` is the type for the pointer produced from calls to `IterableTypeIterator_next(...)`.  
`for_each_enumerate(ElementType, EnumeratedElement, IterableType, IterableInstance)`  
&emsp;macro allows for sequential iteration through an `Iterable` object but associates a sequence number for each object in the `Iterable`.  
&emsp;`IterableType` is the underlying object type that the macro will iterate over. For `Iterator`s that do not have a global struct defined of such a type, simply remove `Iterator` from the name, e.g. to use a `FileLineIterator`, `IterableType` should be `FileLine`.  
&emsp;`IteratbleInstance` is a pointer to an `IterableType`.  
&emsp;`ElementType` is the type for the pointer produced from calls to `IterableTypeIterator_next(...)`.  
&emsp;`EnumeratedElement` is the variable name for an unnamed struct with contents `{size_t i; ElementType * val;}`. `i` is this enumeration

The macros produce an extra level of indirection for both the `Iterable` and its `Element`s. Specifically, `for_each` and `for_each_enumerate`, the `Iterator`s will produce instances of `ElementType *` and not `ElementType` itself. This can be a little confusing in some cases, the primary example in this library being for iterating over lines in a file. The appropriate `ElementType` is `char` and NOT `char *`. If `ElementType` is set to `char *`, then instances of `Element` will actually be of type `char **`. This largely is done to have symmetry with the `Iterable` object.

`Iterable` objects `OBJECT` are those that store some collection of variables of type `TYPE` and have the following minimum functionality defined:
  * `OBJECTIterator` - a struct to hold the state of the `Iterator`.  
  * `OBJECTIterator * OBJECTIterator_iter(...)` - a variadic constructor function, usually a macro, to create new instances of `OBJECTITerator`.
  * `TYPE * OBJECTIterator_next(OBJECTIterator *)` - a function to provide then next element from the `Iterable` `OBJECT`.
  * `enum iterator_status OBJECTIterator_stop(OBJECTIterator *)` - a function to determine whether iteration has ended.

## Examples

sample.txt
```
Yo
quiero
Taco
Bell

```

### `FileLineIterator` with `for_each`
```
for_each(char, line, FileLine, "sample.txt") {
    printf("line: %s", line);
}
```
produces:
```
line: Yo
line: quiero
line: Taco
line: Bell
```

### `LineIterator` with `for_each`
```
FILE * _file = fopen("sample.txt", "rb");
for_each(char, line, Line, _file) {
    printf("line: %s", line);
}
fclose(_file);
```
produces:
```
line: Yo
line: quiero
line: Taco
line: Bell
```

### `FileLineIterator` with `for_each_enumerate`
```
for_each_enumerate(char, line, FileLine, "sample.txt") {
    printf("%llu: %s", line.i, line.val);
}
```
produces:
```
0: Yo
1: quiero
2: Taco
3: Bell
```

### `LineIterator` with `for_each_enumerate`
```
FILE * _file = fopen("sample.txt", "rb");
for_each_enumerate(char, line, Line, _file) {
    printf("%llu: %s", line.i, line.val);
}
fclose(_file);
```
produces:
```
0: Yo
1: quiero
2: Taco
3: Bell
```