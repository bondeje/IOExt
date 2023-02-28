/*
Requirements: In order to use the facilities in this header, it is assumed that for a given OBJECT
type with elements of type TYPE, the following structures and functions are defined with the 
explicit signatures. Such OBJECTs are called ITERABLE

structs:
OBJECTIterator {
    // rest of struct is opaque
}

functions:
// required for for_each
OBJECTIterator * OBJECTIterator_iter(variable number of arguments to be supplied) {
    // allocates the memory for the OBJECTITerator
}
TYPE * OBJECTIterator_next(OBJECTIterator * obj_iter) {
    // returns the subsequent elements (one at a time) of the ITERABLE OBJECT
    // commonly, this accounts for identifying when no more elements are available, usually by
    // failing and returning NULL.
}
iteration_status OBJECTIterator_stop(OJBECTIterator * obj_iter) {
    // returns an element of iteration_status enum
    // a useful paradigm is that if the OBJECTIterator requires memory allocation, use this function, 
    // which if identifies ITERATOR_STOP, to call OBJECTIterator_del. See NOTES below   
}

NOTES:
1) OBJECT itself can be an iterator and highly suggested to implement, but to keep things simple, 
    OBJECTIteratorIterator_iter should return just OBJECTIterator *
2) Generally and following the conventions in my other libraries, it is common to have functions
    allowing full configurations:
OBJECTIterator * OBJECTIterator_new(args...) {
    // full OBJECTIterator dynamic allocation. OBJECTIterator_iter can just be a call with only a 
    // single object and then default remaining arguments
    // usually calls OBJECTIterator_init before returning
}
void OBJECTIterator_init(OBJECTITerator * obj_iter, args...) {
    // full OBJECTIterator configuration. If using a dynamic memory allocation, this is called at
    // the end of OBJECTIterator_new. This function is separate to allow for a static memory
    // allocation paradigm. For such cases, the facilities here will not work and a more explicit
    // allocation preceding this call is needed before while/for loops
}
void OBJECTIterator_del(OBJECTITerator * obj_iter) {
    // free all dynamically allocated objects
}
3) The implemented behavior of OBJECTIterator_stop for some OBJECTs to de-allocate the underlying
    // iterator may be against convention for some, but it is required for implementing a for_each
    // MACRO. The simple "solution" to this behavior for OBJECTIterator_next output NULL or an 
    // appropriate stop condition and use that to implement their own stop
*/

/* 
currently ITERATION_DELAY unused, but the purposes would be to allow ending in a way not conformant with the standard while & for loop examples

for example, the standard conditions above work with all the standard constructions in this module

ObjectIterator * obj_iter_instance = ObjectIterator_simple(obj_to_iterate);

ElementType element_instance = ObjectIterator_next(obj_iter_instance);
while (!ObjectIterator_stop(obj_iter_instance)) {
    element_instance = ObjectIterator_next(obj_iter_instance)
}

-- or --

for (ElementType element_instance = ObjectIterator_start(obj_iter_instance); !ObjectIterator_stop(obj_iter_instance); element_instance = ObjectIterator_next(obj_iter_instance)) {

}

but say you want to change the point at which the stop condition is checked, e.g. at the end of the loop like a do {} while();, then you would have to implement a delay in the stop that basically counts down to stop

*/

#include <stdio.h>

#ifndef ITERATORS_H
#define ITERATORS_H

#ifndef LINE_BUFFER_SIZE
#define LINE_BUFFER_SIZE BUFSIZ
#endif // LINE_BUFFER_SIZE

enum iterator_status {
    ITERATOR_FAIL = -1,
    ITERATOR_GO,
    ITERATOR_STOP,
    ITERATOR_DELAY,
};

// The combination (objtype, inst) must be unique within a local scope
// variadic argument are the arguments in available constructors
#define for_each(insttype, inst, objtype, ...)								                \
objtype##Iterator * objtype##_##inst##_iter = objtype##Iterator_iter(__VA_ARGS__);	\
for (insttype * inst = (insttype *) objtype##Iterator_next(objtype##_##inst##_iter); objtype##_##inst##_iter && !objtype##Iterator_stop(objtype##_##inst##_iter); inst = (insttype *) objtype##Iterator_next(objtype##_##inst##_iter))

// The combination (objtype, inst) must be unique within a local scope as well as inst itself as a variable
// variadic argument are the arguments in available constructors
#define for_each_enumerate(insttype, inst, objtype, ...)                                    \
objtype##Iterator * objtype##_##inst##_iter = objtype##Iterator_iter(__VA_ARGS__);	\
for (struct {size_t i; insttype * val;} inst = { 0, (insttype *) objtype##Iterator_next(objtype##_##inst##_iter)}; objtype##_##inst##_iter && !objtype##Iterator_stop(objtype##_##inst##_iter); inst.i++, inst.val = (insttype *) objtype##Iterator_next(objtype##_##inst##_iter))

#endif // ITERATORS_H