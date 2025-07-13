#!/bin/bash

# OptiWeave Code Formatting Script
# Usage: ./scripts/format.sh [options]

set -e

# Configuration
CLANG_FORMAT_VERSION="${CLANG_FORMAT_VERSION:-15}"
DRY_RUN=false
CHECK_ONLY=false
VERBOSE=false
INCLUDE_TESTS=true
INCLUDE_EXAMPLES=true

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    cat << EOF
OptiWeave Code Formatting Script

Usage: $0 [OPTIONS] [FILES...]

OPTIONS:
    -h, --help              Show this help message
    -c, --check             Check formatting without modifying files
    -d, --dry-run           Show what would be formatted without changes
    -v, --verbose           Enable verbose output
    --no-tests              Skip formatting test files
    --no-examples           Skip formatting example files
    --version VERSION       Use specific clang-format version (default: $CLANG_FORMAT_VERSION)

EXAMPLES:
    $0                      # Format all source files
    $0 -c                   # Check if files are properly formatted
    $0 src/main.cpp         # Format specific file
    $0 --no-tests           # Format only main source, skip tests

EOF
}

# Parse command line arguments
FILES_TO_FORMAT=()

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--check)
            CHECK_ONLY=true
            shift
            ;;
        -d|--dry-run)
            DRY_RUN=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --no-tests)
            INCLUDE_TESTS=false
            shift
            ;;
        --no-examples)
            INCLUDE_EXAMPLES=false
            shift
            ;;
        --version)
            CLANG_FORMAT_VERSION="$2"
            shift 2
            ;;
        -*)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
        *)
            FILES_TO_FORMAT+=("$1")
            shift
            ;;
    esac
done

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Change to project root
cd "$PROJECT_ROOT"

# Find clang-format
CLANG_FORMAT=""
for version in "$CLANG_FORMAT_VERSION" ""; do
    cmd="clang-format${version:+-$version}"
    if command -v "$cmd" &> /dev/null; then
        CLANG_FORMAT="$cmd"
        break
    fi
done

if [[ -z "$CLANG_FORMAT" ]]; then
    log_error "clang-format not found"
    log_info "Please install clang-format version $CLANG_FORMAT_VERSION or later"
    log_info "  Ubuntu/Debian: sudo apt install clang-format-$CLANG_FORMAT_VERSION"
    log_info "  macOS: brew install clang-format"
    exit 1
fi

# Get clang-format version
ACTUAL_VERSION=$($CLANG_FORMAT --version | grep -o '[0-9]\+\.[0-9]\+' | head -1)
log_info "Using $CLANG_FORMAT (version $ACTUAL_VERSION)"

# Check if .clang-format file exists
if [[ ! -f ".clang-format" ]]; then
    log_warning ".clang-format file not found, creating default configuration"
    cat > .clang-format << 'EOF'
---
Language: Cpp
BasedOnStyle: Google
AccessModifierOffset: -2
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignEscapedNewlines: Left
AlignOperands: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: All
AllowShortIfStatementsOnASingleLine: true
AllowShortLoopsOnASingleLine: true
AlwaysBreakAfterDefinitionReturnType: None
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: true
AlwaysBreakTemplateDeclarations: true
BinPackArguments: true
BinPackParameters: true
BraceWrapping:
  AfterClass: false
  AfterControlStatement: false
  AfterEnum: false
  AfterFunction: false
  AfterNamespace: false
  AfterObjCDeclaration: false
  AfterStruct: false
  AfterUnion: false
  BeforeCatch: false
  BeforeElse: false
  IndentBraces: false
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
BreakAfterJavaFieldAnnotations: false
BreakStringLiterals: true
ColumnLimit: 100
CommentPragmas: '^ IWYU pragma:'
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: true
DisableFormat: false
ExperimentalAutoDetectBinPacking: false
ForEachMacros: [ foreach, Q_FOREACH, BOOST_FOREACH ]
IncludeCategories:
  - Regex: '^<.*\.h>'
    Priority: 1
  - Regex: '^<.*'
    Priority: 2
  - Regex: '.*'
    Priority: 3
IncludeIsMainRegex: '([-_](test|unittest))?$'
IndentCaseLabels: true
IndentWidth: 4
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: false
MacroBlockBegin: ''
MacroBlockEnd: ''
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
ObjCBlockIndentWidth: 2
ObjCSpaceAfterProperty: false
ObjCSpaceBeforeProtocolList: false
PenaltyBreakBeforeFirstCallParameter: 1
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 200
PointerAlignment: Left
ReflowComments: true
SortIncludes: true
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles: false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Auto
TabWidth: 4
UseTab: Never
EOF
    log_success "Created default .clang-format configuration"
fi

# Find files to format if not specified
if [[ ${#FILES_TO_FORMAT[@]} -eq 0 ]]; then
    log_info "Finding source files to format..."
    
    # Core source files
    FILES_TO_FORMAT+=($(find src include -name "*.cpp" -o -name "*.hpp" -o -name "*.h" 2>/dev/null || true))
    
    # Test files
    if [[ "$INCLUDE_TESTS" == true ]]; then
        FILES_TO_FORMAT+=($(find tests -name "*.cpp" -o -name "*.hpp" -o -name "*.h" 2>/dev/null || true))
    fi
    
    # Example files
    if [[ "$INCLUDE_EXAMPLES" == true ]]; then
        FILES_TO_FORMAT+=($(find examples -name "*.cpp" -o -name "*.hpp" -o -name "*.h" 2>/dev/null || true))
    fi
    
    if [[ ${#FILES_TO_FORMAT[@]} -eq 0 ]]; then
        log_warning "No source files found to format"
        exit 0
    fi
fi

# Filter out non-existent files
EXISTING_FILES=()
for file in "${FILES_TO_FORMAT[@]}"; do
    if [[ -f "$file" ]]; then
        EXISTING_FILES+=("$file")
    elif [[ "$VERBOSE" == true ]]; then
        log_warning "File not found: $file"
    fi
done

FILES_TO_FORMAT=("${EXISTING_FILES[@]}")

if [[ ${#FILES_TO_FORMAT[@]} -eq 0 ]]; then
    log_warning "No valid files to format"
    exit 0
fi

log_info "Found ${#FILES_TO_FORMAT[@]} files to format"

# Format files
MODIFIED_FILES=()
FORMATTING_ERRORS=()

for file in "${FILES_TO_FORMAT[@]}"; do
    if [[ "$VERBOSE" == true ]]; then
        log_info "Processing: $file"
    fi
    
    if [[ "$CHECK_ONLY" == true ]]; then
        # Check if file needs formatting
        if ! $CLANG_FORMAT --dry-run --Werror "$file" &>/dev/null; then
            MODIFIED_FILES+=("$file")
            if [[ "$VERBOSE" == true ]]; then
                log_warning "  File needs formatting: $file"
            fi
        fi
    elif [[ "$DRY_RUN" == true ]]; then
        # Show what would be changed
        if ! $CLANG_FORMAT --dry-run --Werror "$file" &>/dev/null; then
            MODIFIED_FILES+=("$file")
            log_info "  Would format: $file"
            if [[ "$VERBOSE" == true ]]; then
                # Show diff
                diff -u "$file" <($CLANG_FORMAT "$file") || true
            fi
        fi
    else
        # Actually format the file
        if ! $CLANG_FORMAT -i "$file" 2>/dev/null; then
            FORMATTING_ERRORS+=("$file")
            log_error "  Failed to format: $file"
        else
            # Check if file was actually modified
            if [[ -n "$(git status --porcelain "$file" 2>/dev/null)" ]]; then
                MODIFIED_FILES+=("$file")
                if [[ "$VERBOSE" == true ]]; then
                    log_success "  Formatted: $file"
                fi
            fi
        fi
    fi
done

# Summary
echo
log_info "Formatting Summary:"
log_info "  Total files processed: ${#FILES_TO_FORMAT[@]}"
log_info "  Files needing formatting: ${#MODIFIED_FILES[@]}"

if [[ ${#FORMATTING_ERRORS[@]} -gt 0 ]]; then
    log_info "  Formatting errors: ${#FORMATTING_ERRORS[@]}"
fi

if [[ "$CHECK_ONLY" == true ]]; then
    if [[ ${#MODIFIED_FILES[@]} -gt 0 ]]; then
        log_error "The following files need formatting:"
        for file in "${MODIFIED_FILES[@]}"; do
            echo "  $file"
        done
        echo
        log_info "Run './scripts/format.sh' to format these files"
        exit 1
    else
        log_success "All files are properly formatted!"
    fi
elif [[ "$DRY_RUN" == true ]]; then
    if [[ ${#MODIFIED_FILES[@]} -gt 0 ]]; then
        log_info "The following files would be formatted:"
        for file in "${MODIFIED_FILES[@]}"; do
            echo "  $file"
        done
    else
        log_success "All files are already properly formatted!"
    fi
else
    if [[ ${#MODIFIED_FILES[@]} -gt 0 ]]; then
        log_success "Formatted ${#MODIFIED_FILES[@]} files:"
        for file in "${MODIFIED_FILES[@]}"; do
            echo "  $file"
        done
    else
        log_success "All files were already properly formatted!"
    fi
    
    if [[ ${#FORMATTING_ERRORS[@]} -gt 0 ]]; then
        log_error "Failed to format ${#FORMATTING_ERRORS[@]} files:"
        for file in "${FORMATTING_ERRORS[@]}"; do
            echo "  $file"
        done
        exit 1
    fi
fi

# Check for clang-format configuration issues
if [[ "$VERBOSE" == true ]]; then
    log_info "Clang-format configuration check:"
    $CLANG_FORMAT --dump-config > /tmp/optiweave-format-config.yaml
    log_info "  Configuration dumped to /tmp/optiweave-format-config.yaml"
fi

exit 0
