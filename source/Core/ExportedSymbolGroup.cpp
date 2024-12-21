#include "ExportedSymbolGroup.h"

#include <stdexcept>

const char *SymbolKind_ToString(SymbolKind value) {
    switch (value) {
        case SymbolKind::Atom: return "Atom";
        case SymbolKind::Enum: return "Enum";
        case SymbolKind::Class: return "Class";
        case SymbolKind::Struct: return "Struct";
        case SymbolKind::Typedef: return "Typedef";
        case SymbolKind::Function: return "Function";
        case SymbolKind::Variable: return "Variable";
        case SymbolKind::Container: return "Container";
        case SymbolKind::SourceFile: return "SourceFile";
        default: throw std::runtime_error("Unexpected symbol kind");
    }
}
