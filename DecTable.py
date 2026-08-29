from dataclasses import dataclass


@dataclass
class Keyword:
    name: str
    ident: str


@dataclass
class DestableRow:
    row: int
    char: str
    opthen: str
    opelse: str


kwlist = [
    Keyword("AND", "TOKEN_KW_AND"),
    Keyword("ANDALSO", "TOKEN_KW_ANDALSO"),
    Keyword("CASE", "TOKEN_KW_CASE"),
    Keyword("CONST", "TOKEN_KW_CONST"),
    Keyword("CONTINUE", "TOKEN_KW_CONTINUE"),
    Keyword("DECLARE", "TOKEN_KW_DECLARE"),
    Keyword("DIM", "TOKEN_KW_DIM"),
    Keyword("DO", "TOKEN_KW_DO"),
    Keyword("ELSE", "TOKEN_KW_ELSE"),
    Keyword("ELSEIF", "TOKEN_KW_ELSEIF"),
    Keyword("END", "TOKEN_KW_END"),
    Keyword("EXIT", "TOKEN_KW_EXIT"),
    Keyword("FALSE", "TOKEN_KW_FALSE"),
    Keyword("FOR", "TOKEN_KW_FOR"),
    Keyword("FUNCTION", "TOKEN_KW_FUNCTION"),
    Keyword("GLOBAL", "TOKEN_KW_GLOBAL"),
    Keyword("IF", "TOKEN_KW_IF"),
    Keyword("INPUT", "TOKEN_KW_INPUT"),
    Keyword("IS", "TOKEN_KW_IS"),
    Keyword("LEN", "TOKEN_KW_LEN"),
    Keyword("LOOP", "TOKEN_KW_LOOP"),
    Keyword("MOD", "TOKEN_KW_MOD"),
    Keyword("NEXT", "TOKEN_KW_NEXT"),
    Keyword("NOT", "TOKEN_KW_NOT"),
    Keyword("OR", "TOKEN_KW_OR"),
    Keyword("ORELSE", "TOKEN_KW_ORELSE"),
    Keyword("PRINT", "TOKEN_KW_PRINT"),
    Keyword("REF", "TOKEN_KW_REF"),
    Keyword("REM", "TOKEN_KW_REM"),
    Keyword("RETURN", "TOKEN_KW_RETURN"),
    Keyword("SELECT", "TOKEN_KW_SELECT"),
    Keyword("STATIC", "TOKEN_KW_STATIC"),
    Keyword("STEP", "TOKEN_KW_STEP"),
    Keyword("SUB", "TOKEN_KW_SUB"),
    Keyword("THEN", "TOKEN_KW_THEN"),
    Keyword("TO", "TOKEN_KW_TO"),
    Keyword("TRUE", "TOKEN_KW_TRUE"),
    Keyword("UNTIL", "TOKEN_KW_UNTIL"),
    Keyword("WHILE", "TOKEN_KW_WHILE"),
    Keyword("XOR", "TOKEN_KW_XOR"),
]

next_row = 0
table = []


def add_row(char):
    global next_row
    row = DestableRow(next_row, char, "0", "0")
    table.append(row)
    next_row += 1
    return row


def process(names):
    prev = None

    emptys = [it for it in names if len(it.name) == 0]
    if len(emptys) > 1:
        raise ValueError("Duplicate words")
    if len(emptys) == 1:
        prev = add_row("#")
        prev.opthen = emptys[0].ident

    chars = sorted(set(it.name[0] for it in names if len(it.name) > 0))
    for char in chars:
        row = add_row(char)
        row.opthen = str(next_row)
        if prev:
            prev.opelse = str(row.row)
        nextlist = [Keyword(it.name[1:], it.ident) for it in names if it.name.startswith(char)]
        process(nextlist)
        prev = row


def find(name):
    row = 0
    pos = 0
    name = name + "#"
    while True:
        if table[row].char == name[pos]:
            if name[pos] == "#":
                return table[row].opthen
            pos += 1
            row = int(table[row].opthen)
        else:
            if table[row].opelse == "0":
                return None
            row = int(table[row].opelse)


process(kwlist)

print("Count:", len(table))

for row in table:
    # print(f"{row.row:3}  '{row.char}'  {row.opthen}  {row.opelse}")
    if row.char == "#":
        print(f"{{'\\0', {row.opthen}, {row.opelse}}},")
    else:
        print(f"{{'{row.char}',  {row.opthen}, {row.opelse}}},")

# print("Table size: ", len(table) * 3, " bytes")
# print("Raw strings: ", sum(len(it.name) + 1 for it in kwlist), " bytes")
# print("IF", find("IF"))
# print("QWERT", find("QWERT"))
# print("CONTINUE", find("CONTINUE"))
