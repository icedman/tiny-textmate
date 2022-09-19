# tiny textmate grammar parser

## goals

1. tiny
2. independent json reader
3. line by line parsing
4. serializable parser state
5. fast

## parse algorithm (based on textpow)

```js
top = stack_top()
start = buffer_start
end = buffer_end

do {
    if (top.patterns) {
        pattern_match = match_first_pattern()
    }

    if (top.end) {
        end_match = match_end()
    }

    if (end_match && (!pattern_match || pattern_match.first <= end_match.first )) {
        pattern_match = end_match
        start = pattern_match.first
        end = pattern_match.last

        process_captures()

        stack_pop()
        top = stack_top()
    } else {
        if (!pattern_match) break

        if (pattern_match == begin_pattern) {
            process_captures()

            stack_push(pattern_match)
            top = stack_top()
        } else if (pattern_match == match_pattern) {
            process_captures()
        }
    }

    position = end

} while(true)

```