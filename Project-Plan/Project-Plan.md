# Project Plan

## Precursor

<https://github.com/markpmlim/EdAsm>

## Basic Concept

We want to reimplement this editor-assembler as closely to identical as possible

## Rules

The project will use C++ on Linux. Insofar as the original does direct screen addressing, we can use ncurses.

Apart from that, we will follow the original source as closely as possible, including trying to keep all the original labels, comments, and bugs. We will try to follow the original code structure as closely as possible, and implement the original algorithms as exactly as possible.

Exceptions are:

- when the interfaces to linux and ncurses demand something different
- we will include all code in a single executable
- when we have specific labels for lower and higher bytes, these can be replaced by one label for either a 16 bit word, or an address, as appropriate
- the original code uses Xabcd labels for linking to other pieces of the code, and Labcd labels inside separate pieces; these are equivalent
- if there is some other reason that makes it impossible to follow the old code
