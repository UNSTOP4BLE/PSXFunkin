import re
shit = ''
with open('shit.txt', 'r') as file:
    shit = file.read()

def removeCCppComment( text ) :

    def blotOutNonNewlines( strIn ) :  # Return a string containing only the newline chars contained in strIn
        return "" + ("\n" * strIn.count('\n'))

    def replacer( match ) :
        s = match.group(0)
        if s.startswith('/'):  # Matched string is //...EOL or /*...*/  ==> Blot out all non-newline chars
            return blotOutNonNewlines(s)
        else:                  # Matched string is '...' or "..."  ==> Keep unchanged
            return s

    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )

    return re.sub(pattern, replacer, text)


shit = removeCCppComment(shit)

shit = shit.replace("{", "[")
shit = shit.replace("}", "]")

print(shit);