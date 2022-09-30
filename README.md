# tiny textmate grammar parser

## goals

1. tiny
2. independent json reader
3. line by line parsing
4. serializable parser state
5. fast

## parse algorithm (based on textpow)

```c
// a syntax is a regex pattern to match (3 cases: begin, end, match; there is a 'while'?)
struct syntax_t {
    regex_t begin
    regex_t end
    regex_t match
    // regexes may find the start or the end of a region
    // or it may be simple match

    // when a match is found, its region may be assigned a scope
    string scope_name
    // or it may proceed to match other patterns
    syntax_t patterns[]
    // a syntax contains therefore a tree structure of syntaxes

    // regex matches may capture regions and assign scope upon them
    regex_match_t captures

    // syntax may also b an 'include' which references another defined syntax
    syntax_t *include
};

// parsing a buffer requires 1. stack of parser states; and 2. a buffer range
// the stack of parser states at a minimum includes a syntax.
// it may also include matched results for each syntax

top = stack_top()
start = buffer_start
end = buffer_end

do {
    if (top.patterns) {
        pattern_match = match_first_pattern()
    }

    if (top.end) {
        // match_end could consider captures from match_begin?
        end_match = match_end()
    }

    // pattern matches are prioritizes over ending matches
    if (end_match && (!pattern_match || pattern_match.first <= end_match.first )) {
        pattern_match = end_match
        start = pattern_match.first
        end = pattern_match.last

        // when an end is found for the top syntax

        // captures are processed. it may expand a scope (keyword.$1) 
        process_captures()

        // the top syntax is discarded
        stack_pop()
        top = stack_top()
    } else {
        if (!pattern_match) break

        if (pattern_match == begin_pattern) {
            // when a begin is found

            process_captures()

            // it is added to the stack
            // and its patterns will be considered on the next loop, 
            // until the syntax is discarded at end
            stack_push(pattern_match)
            top = stack_top()
        } else if (pattern_match == match_pattern) {

            // a simple match processes only captures
            process_captures()
        }
    }

    position = end

} while(true)

```
# TODO

* proper 'end' regex - recompile
* parser state serialize

# Reference

https://macromates.com/manual/en/language_grammars
https://www.apeth.com/nonblog/stories/textmatebundle.html
