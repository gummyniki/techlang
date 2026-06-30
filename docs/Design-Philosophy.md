# Techlang Design Philosphy

## Pointers

Techlang favors explicit, low-level semantics over hidden 
reference passing. Pointers are first-class and visible at 
both the declaration site (`int*`) and the call site 
(`x.address`) — there's no implicit `inout`/`by-ref` magic.
