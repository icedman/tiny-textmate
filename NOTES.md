# Textmate grammar structure

{
    patterns: [
        Pattern { match, captures, name },
        Pattern { begin/end, captures, name, patterns: [ Pattern, Pattern ... ] }
        Pattern { begin/while, captures, name, patterns: [ ... ] },
        ...
    ],

    repository: [
        Pattern,
        Pattern,
        ...
    ]
}

# Some Textmate features or quirks

## The structure of pattern is that it may be a simple match or begin/end/while match;

- This is similar to vim's match & region. However textmate rolled the structure into one;

## Begin/While

- The region continues as long as While pattern is matched. I think Begin/While was added for efficiency as it breaks the region matching immediately if While match fails. Whereas, in Begin/End the region check may still continue even if the End match is found so long as other Include/Contained patterns are found at earlier locations;

## Begin/End/While Patterns in textmate may include other patterns listed under a repository table.

- This is similar to vim's contains-contained. 
- Patterns may include itself by including its own name under its pattern list
- Or may include the grammar document itself ($self)
- Or may include other external grammar document

# The Basic Match

Pattern
{
    match // (regular expression)
    name // (if regex is matched, this name is assigned to entire matched span)
}

Similar to syn's match with this difference:

`name` may be declared as `entity.tag.$1`, where $1 is replaced with the regular expression submatch, 1 being an index or location of the submatch. So it may for example resolve to `entity.begin.HTML` or `entity.begin.BODY`

# With Capatures

Pattern
{
    match // (regular expression with submatches)
    name // (if regex is matched, this name is assigned to entire matched span)
    capture: {
        1: { name }
        2: { name }
    }
}

A capture/submatch may itself be assigned a name - which may have to be resolved if it contains $1, $2, etc

```c
int main(int argc, char **argv)
```

Pattern {
    match (int|void|char)\s([a-zA-Z]+)\(
    name 'function call'
    captures: {
        1: { name: 'return.type.$1' },
        2: { name: 'name.function' },
    }
}

Complication on Captures

Captures may itself include patterns

```c
void MyClass::main(
```

Pattern
{
    match (int|void|char)\s([a-zA-Z]+::[a-zA-Z]+)\(
    captures: {
        2: {
            match: ([a-zA-Z]+)::([a-zA-Z]+),
            captures: {
                1: { name: 'class' },
                2: { name: 'class.member.function' }
            }
        }
    }
},

Vim regex allows up to 10? submatches. There is no such limit in Textmate. One pattern in cpp has 81! captures.
