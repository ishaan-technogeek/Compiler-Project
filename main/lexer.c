/* Group 25 :
Members:
    Ishaan Pavan Gupta 2022A7PS1180P
    Aayush Gupta       2022A7PS0088P
    Saksham Jain       2022A7PS0132P
    Mudit Gupta        2022A7PS0178P
    Aditi Bansal       2022A7PS0053P
    Kshitiz Gupta      2022A7PS0057P
 */
#include "lexerDef.h"
#include "interface.h"
#include "keyword_table.h"
#include <string.h>

// DFA state
int dfaState = 0;
// Tracks the line number
int currentLine = 1;
// Flag to check if input is exhausted, ensuring EOF is returned if the file is depleted
int isInputExhausted = 0;
// Flag to prevent double counting '\n' after retraction
int hasRetracted = 0;

int fileDesc =0;

void initializeTwinBuffer(TwinBuffer* B, int fd) {
    B->activeBuffer = 0;
    B->nextBuffer = 1;
    B->startLexeme = NULL;
    B->lookahead = NULL;

    fileDesc = fd;
}

TwinBuffer* initializeLexer(int fd) {
    // Allocate memory for TwinBuffer
    TwinBuffer* B = (TwinBuffer*)malloc(sizeof(TwinBuffer));
    if (B == NULL) {
        printf("Memory allocation failed for TwinBuffer\n");
        return NULL;
    }

    // Reset global state
    isInputExhausted = 0;
    currentLine = 1;
    hasRetracted = 0;
    dfaState = 0;

    // Initialize buffer
    initializeTwinBuffer(B, fd);
    
    return B; // Return allocated and initialized buffer
}



// Replace the existing TerminalID initialization with:
char* TerminalID[] = {
    "TK_ASSIGNOP",
    "TK_COMMENT",
    "TK_FIELDID",
    "TK_ID",
    "TK_NUM",
    "TK_RNUM",
    "TK_FUNID",
    "TK_RUID",
    "TK_WITH",
    "TK_PARAMETERS",
    "TK_END",
    "TK_WHILE",
    "TK_UNION",
    "TK_ENDUNION",
    "TK_DEFINETYPE",
    "TK_AS",
    "TK_TYPE",
    "TK_MAIN",
    "TK_GLOBAL",
    "TK_PARAMETER",
    "TK_LIST",
    "TK_SQL",
    "TK_SQR",
    "TK_INPUT",
    "TK_OUTPUT",
    "TK_INT",
    "TK_REAL",
    "TK_COMMA",
    "TK_SEM",
    "TK_COLON",
    "TK_DOT",
    "TK_ENDWHILE",
    "TK_OP",
    "TK_CL",
    "TK_IF",
    "TK_THEN",
    "TK_ENDIF",
    "TK_READ",
    "TK_WRITE",
    "TK_RETURN",
    "TK_PLUS",
    "TK_MINUS",
    "TK_MUL",
    "TK_DIV",
    "TK_CALL",
    "TK_RECORD",
    "TK_ENDRECORD",
    "TK_ELSE",
    "TK_AND",
    "TK_OR",
    "TK_NOT",
    "TK_LT",
    "TK_LE",
    "TK_EQ",
    "TK_GT",
    "TK_GE",
    "TK_NE",
    "TK_ERR",
    "TK_DOLLAR",
    "TK_EPS"
};
 
char* getTerminal(int enumId) {
    return TerminalID[enumId];
}



int getStream(int fd, TwinBuffer* B) {
    if (isInputExhausted)
        return EOF;
    fileDesc = fd;
    
    memset(B->buffPair[B->nextBuffer],EOF ,BUFFER_SIZE);

    int bytesRead = read(fd,B->buffPair[B->nextBuffer], BUFFER_SIZE);

    B->nextBuffer = 1 - B->nextBuffer;
    B->activeBuffer = 1 - B->activeBuffer;
    
    if(bytesRead == -1)
    {
        printf("Error: Input Buffers failed to be loaded.\n");
        return -2;
    }
    if(bytesRead ==0 || bytesRead < BUFFER_SIZE)
    {
        isInputExhausted = 1;
    }
    return 1;

}

char getNextChar(TwinBuffer* B) {
    if (B->startLexeme == NULL && B->lookahead == NULL) {
        int result = getStream(fileDesc, B);
        if (result == -1 || result == -2) {
            return EOF;
        }
        B->startLexeme = B->buffPair[B->activeBuffer];
        B->lookahead = B->buffPair[B->activeBuffer];

        //to prevent buffer overflow 
    if (B->lookahead >= B->buffPair[B->activeBuffer] + BUFFER_SIZE) {
        printf("!!!!!!!!!!!!!!!!!!!!! Buffer overflow\n");
        //  Lexeme spans across buffers, shift into next buffer
        size_t firstPartSize = B->buffPair[0] + BUFFER_SIZE - B->startLexeme; // Remaining in buffer1

        // Shift first part of lexeme into next buffer
        memcpy(B->buffPair[B->nextBuffer], B->startLexeme, firstPartSize);

        // Load new buffer and update pointers
        int result = getStream(fileDesc, B);
        if (result == -1 || result == -2) {
            return EOF;
        }

        B->lookahead = B->buffPair[B->activeBuffer];  // Reset `lookahead`
        B->startLexeme = B->buffPair[B->activeBuffer];  // Shifted lexeme now in `nextBuffer` 
    }

    char* currChar = B->lookahead;
    B->lookahead++;

    if (hasRetracted == 0 && *currChar == '\n') {
        currentLine++;
    }

    if (hasRetracted == 1)
        hasRetracted = 0;

    return *currChar;
    }

    char* currChar = B->lookahead;
    if (currChar - B->buffPair[B->activeBuffer] == BUFFER_SIZE - 1) {
        int result = getStream(fileDesc, B);
        if (result == -1 || result == -2) {
            return EOF;
        }
        B->lookahead = B->buffPair[B->activeBuffer];
        B->startLexeme = B->buffPair[B->activeBuffer];
    } else if (*currChar == EOF) {
        return EOF;
    }
    else
        (B->lookahead)++;

    if(hasRetracted == 0 && *currChar == '\n')
        currentLine++;

    if(hasRetracted == 1)
        hasRetracted = 0;

    return *currChar;
}

void acceptLexeme(TwinBuffer* B) {
    B->startLexeme = B->lookahead;
}

void retractLexeme(TwinBuffer* B, int count) {
    while (count > 0) {
        (B->lookahead)--;
        count--;
    }
    if (B->lookahead < B->startLexeme) {
        printf("Error: lookahead moved before startLexeme! Resetting...\n");
        B->startLexeme = B->lookahead;
    }
    hasRetracted = 1;
}

int rangeMatch(char ch,char start, char end) {
    if(ch >= start && ch <= end)
        return 1;
    return 0;
}

char* copyString(const char *start, const char *end) { 
    if (start == NULL || end == NULL) {
        printf("Error: NULL pointer access in copyString\n");
        return NULL;
    }

    // If lexeme spans across buffers
   if (end < start) {  //Fixes lexemes spanning two buffers
    printf("Error: Invalid lexeme boundary! startLexeme: %p, lookahead: %p\n", start, end);
        return NULL;
    }
    //normal case
    size_t len = end - start;
    char* result = (char*)malloc(len + 1);
    if (result) {
        strncpy(result, start, len);
        result[len] = '\0';
    }
    return result;  // Allocates and copies substring
}

Token* createToken(Token* token, TokenName tokenType, char* lexeme, int lineNum, int isNumeric, Value* value) {
    token->TOKEN_NAME = tokenType;
    token->LINE_NO = lineNum;
    token->IS_NUMBER = isNumeric;
    token->LEXEME = lexeme;
    token->VALUE = value;
    return token;
}

int stringToInteger(char* str) {
    int num;
    sscanf(str, "%d", &num);
    return num;
}

float stringToFloat(char* str) {
    float f;
    sscanf(str,"%f",&f);
    return f;
}

void removeComments(char* sourceFile, TwinBuffer* B, char* outputFile) {
    int src = open(sourceFile, O_RDONLY);
    // int out = open(outputFile, O_CREAT | O_WRONLY | O_TRUNC);
     int out =1;
    // Set the file descriptor
    fileDesc = src;
    if(src == -1 || out == -1) {
        printf("Error opening file\n");
        return;
    }
    int state = 0;
    char ch;
    while ((ch = getNextChar(B)) != EOF) {
        switch (state) {
            case 0:
                if (ch == ' ' || ch == '\f' || ch == '\r' || ch == '\t' || ch == '\v') {                   
                    write(out, &ch, 1);
                    state = 0;
                } else if (ch == '%') {
                    state = 3;
                } else if (ch == '\n') {
                    write(out, &ch, 1);
                    state = 0;
                } else {
                    write(out, &ch, 1);
                    state = 2;
                }
                break;
            case 2:
                write(out, &ch, 1);
                if (ch == '\n')
                    state = 0;
                break;
            case 3: //Inside comment
                if (ch == '\n') {
                    // write(out, &ch, 1); // Do not write the newline character
                    state = 0;
                }
                break;
        }
    }
    // close(out);
    close(src);
}

Token* getToken(TwinBuffer* B){
    dfaState=0;
    char c = '?';
    Token* token = (Token*)malloc(sizeof(Token));
    int errorType;

    while(1){
        
            if(c == EOF)
            return NULL;

        switch(dfaState){
            case 0: {
                c = getNextChar(B);
                if(c == '<'){
                    dfaState=28;
                }else if(c == '#'){
                    dfaState=36;
                }else if((c >= 'b' && c <='d')){
                    dfaState=53;
                }else if(c == 'a' || (c >='e' && c<= 'z')){
                    dfaState=51;
                }else if(c == '_'){
                    dfaState=32;
                }else if(c == '['){
                    dfaState=6;
                }else if(c == ']'){
                    dfaState=7;
                }else if(c == ','){
                    dfaState=8;
                }else if(c == ';'){
                    dfaState=9;
                }else if(c == ':'){
                    dfaState=10;
                }else if(c == '.'){
                    dfaState=11;
                }else if(c == '('){
                    dfaState=12;
                }else if(c == ')'){
                    dfaState=13;
                }else if(c == '+'){
                    dfaState=14;
                }else if(c == '-'){
                    dfaState=15;
                }else if(c == '*'){
                    dfaState=16;
                }else if(c == '/'){
                    dfaState=17;
                }else if(c == '~'){
                    dfaState=24;
                }else if(c == '!'){
                    dfaState=62;
                }else if(c == '>'){
                    dfaState=25;
                }else if(c == '='){
                    dfaState=60;
                }else if(c == '@'){
                    dfaState=21;
                }else if(c == '&'){
                    dfaState=18;
                }else if(c == '%'){
                    dfaState=5;
                }else if(rangeMatch(c, '0','9')){
                    dfaState=39;
                }else if(c == ' ' || c == '\f' || c == '\r' || c == '\t' || c == '\v'){
                    B->startLexeme++;
                    dfaState=0;
                }else if(c == '\n'){
                    B->startLexeme++;
                    //dfaState=59;
                    dfaState=0;
                }else if(c == EOF){
                    return NULL;
                }else{
                    printf("Line %d : Cannot recognize character %c \n", currentLine, c);
                    //Throw lexical error !
                    errorType=6;
                    dfaState=64;
                }
                break;
            }
            case 14: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_PLUS,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 15: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_MINUS,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 16: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_MUL,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 17: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_DIV,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 12: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_OP,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 13: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_CL,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 6: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_SQL,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 7: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_SQR,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 8: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_COMMA,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 11: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_DOT,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 10: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_COLON,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 9: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_SEM,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 24: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_NOT,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 62: {
                c = getNextChar(B);
                if(c == '=') {
                    dfaState=63;
                }
                else {
                    // Throw Lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead - sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for != ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 63: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_NE,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 28: {
                c = getNextChar(B);
                if(c == '-') {
                    dfaState = 2;
                }
                else if(c == '=') {
                    dfaState = 30;
                }
                else {
                    dfaState = 29;
                }
                break;
            }
            case 2: {
                c = getNextChar(B);
                if(c == '-') {
                    dfaState = 3;
                }
                else {
                    dfaState = 65;
                }
                break;
            }
            case 65: {
                retractLexeme(B, 2);
                char* lex = copyString(B->startLexeme,B->lookahead);

                if(c == '\n')
                    createToken(token,TK_LT,lex,currentLine-1,0,NULL);
                else
                    createToken(token,TK_LT,lex,currentLine,0,NULL);

                acceptLexeme(B);
                return token;
                break;
            }
            case 3: {
                c = getNextChar(B);
                if(c == '-') {
                    dfaState = 4;
                }
                else {
                    // Throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for <--- ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 4: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_ASSIGNOP,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 30: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_LE,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 29: {
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);

                if(c == '\n')
                    createToken(token,TK_LT,lex,currentLine-1,0,NULL);
                else
                    createToken(token,TK_LT,lex,currentLine,0,NULL);

                acceptLexeme(B);
                return token;
                break;
            }
            case 25: {
                c = getNextChar(B);
                if(c == '=') {
                    dfaState = 27;
                }
                else {
                    dfaState = 26;
                }
                break;
            }
            case 27: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_GE,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 26: {
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                if(c == '\n')
                    createToken(token,TK_GT,lex,currentLine-1,0,NULL);
                else
                    createToken(token,TK_GT,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 60: {
                c = getNextChar(B);
                if(c == '=') {
                    dfaState = 61;
                }
                else {
                    // Throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for == ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 61: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_EQ,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 21: {
                c = getNextChar(B);
                if(c == '@') {
                    dfaState = 22;
                }
                else {
                    // Throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for @@@ ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 22: {
                c = getNextChar(B);
                if(c == '@') {
                    dfaState = 23;
                }
                else {
                    // Throw lexical
                    char* pattern = copyString(B->startLexeme,B->lookahead);
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for @@@ ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 23: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_OR,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 18: {
                c = getNextChar(B);
                if(c == '&') {
                    dfaState = 19;
                }
                else {
                    // Throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for &&& ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 19: {
                c = getNextChar(B);
                if(c == '&') {
                    dfaState = 20;
                }
                else {
                    // Throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for &&& ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 20: {
                char* lex = copyString(B->startLexeme,B->lookahead);
                createToken(token,TK_AND,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 5: {
                c = getNextChar(B);
                while(c != '\n' && c != EOF) {
                    c = getNextChar(B);
                }
                char* lex = copyString(B->startLexeme,B->lookahead);
                // Corner case =>  If the character which ended the comment is a \n, then the comment was one below the current currentLine
                if(c == '\n')
                    createToken(token,TK_COMMENT,lex,currentLine-1,0,NULL);
                else
                    createToken(token,TK_COMMENT,lex,currentLine,0,NULL);
                acceptLexeme(B);

                return token;
                break;
            }
            case 53: {
                c = getNextChar(B);
                if(rangeMatch(c,'2','7')) {
                    dfaState = 54;
                }
                else if(rangeMatch(c,'a','z')) {
                    dfaState = 51;
                }
                else {
                    dfaState = 55;
                }
                break;
            }
            case 54: {
                c = getNextChar(B);
                while(rangeMatch(c,'b','d'))
                    c = getNextChar(B);

                if(rangeMatch(c,'2','7'))
                    dfaState = 56;
                else {
                    dfaState = 57;
                }
                break;
            }
            case 55: {
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                if(c == '\n')
                    createToken(token,TK_FIELDID,lex,currentLine-1,0,NULL);
                else
                    createToken(token,TK_FIELDID,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 56: {
                c = getNextChar(B);
                while(rangeMatch(c,'2','7'))
                    c = getNextChar(B);

                if(!rangeMatch(c,'2','7') && !rangeMatch(c,'b','d')) {
                    dfaState = 57;
                }
                else {
                    // Throw lexical error
                    printf("Line %d : two identifers are not allowed back to back without a break ?\n" ,currentLine);
                    errorType = 5;
                    dfaState = 64;
                }
                break;
            }
            case 57: {
                retractLexeme(B, 1);
                int identifierLength = B->lookahead - B->startLexeme + 1;
                if(identifierLength < 2) {
                    printf("Line %d : Identifier length is less than 2\n" , currentLine);
                    errorType = 4;
                    dfaState = 64;
                }
                else if(identifierLength > 20) {
                    printf("Line %d : Identifier length is more than 20\n" ,currentLine);
                    errorType = 4;
                    dfaState = 64;
                }
                else {
                    
                    char* lex = copyString(B->startLexeme,B->lookahead);
                    if (lex == NULL) {
                        printf("Error: copyString returned NULL\n");
                        return NULL;
                    }

                    if(c == '\n')
                        createToken(token,TK_ID,lex,currentLine-1,0,NULL);
                    else
                        createToken(token,TK_ID,lex,currentLine,0,NULL);
                    acceptLexeme(B);
                    return token;
                }
                break;
            }
            case 51: {
                c = getNextChar(B);
                while(rangeMatch(c,'a','z'))
                    c = getNextChar(B);

                dfaState = 52;
                break;
            }
            case 52: {
                // Resolve keyword clash!
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                TokenName tokenName = lookUp(lex);
                if(tokenName == -1) {
                    if(c == '\n')
                        createToken(token,TK_FIELDID,lex,currentLine-1,0,NULL);
                    else
                        createToken(token,TK_FIELDID,lex,currentLine,0,NULL);
                }
                else {
                    if(c == '\n')
                        createToken(token,tokenName,lex,currentLine-1,0,NULL);
                    else
                        createToken(token,tokenName,lex,currentLine,0,NULL);
                }
                acceptLexeme(B);
                return token;
                break;
            }
            case 39: {
                c = getNextChar(B);
                while(rangeMatch(c,'0','9'))
                    c = getNextChar(B);

                if(c == '.') {
                    dfaState = 41;
                }
                else {
                    dfaState = 40;
                }
                break;
            }
            case 40: {
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                Value* val = malloc(sizeof(Value));
                val->INT_VALUE = stringToInteger(lex);
                if(c == '\n')
                    createToken(token,TK_NUM,lex,currentLine-1,1,val);
                else
                    createToken(token,TK_NUM,lex,currentLine,1,val);
                acceptLexeme(B);
                return token;
                break;
            }
            //done till last case
            
            case 41: {
                c = getNextChar(B);
                if(rangeMatch(c,'0','9')) {
                    dfaState = 43;
                }
                else {
                    dfaState = 42;
                }
                break;
            }
            case 42: {
                retractLexeme(B,2);
                char* lex = copyString(B->startLexeme,B->lookahead);
                Value* val = malloc(sizeof(Value));
                val->INT_VALUE = stringToInteger(lex);
                if(c == '\n')
                    createToken(token,TK_NUM,lex,currentLine-1,1,val);
                else
                    createToken(token,TK_NUM,lex,currentLine,1,val);
                acceptLexeme(B);
                return token;
                break;
            }
            case 43: {
                c = getNextChar(B);
                if(rangeMatch(c,'0','9')) {
                    dfaState = 44;
                }
                else {
                    // Throw lexical
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a real number ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 44: {
                c = getNextChar(B);
                if(c == 'E') {
                    dfaState = 45;
                }
                else {
                    dfaState = 50;
                }
                break;
            }
            case 50: {
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                Value* val = (Value*)malloc(sizeof(Value));
                val->FLOAT_VALUE = stringToFloat(lex);
                createToken(token,TK_RNUM,lex,currentLine,2,val);
                acceptLexeme(B);
                return token;
                break;
            }
            case 45:{
                c = getNextChar(B);
                if(c == '+' || c == '-') {
                    dfaState = 46;
                }
                else if(rangeMatch(c,'0','9')) {
                    dfaState = 47;
                }
                else {
                    // Throw lexical
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a real number ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 46: {
                c = getNextChar(B);
                if(rangeMatch(c,'0','9')) {
                    dfaState = 47;
                }
                else {
                    // Throw lexical
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a real number ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 47: {
                c = getNextChar(B);
                if(rangeMatch(c,'0','9')) {
                    dfaState = 48;
                }
                else {
                    // Throw lexical
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a real number ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    retractLexeme(B, 1);
                }
                break;
            }
            case 48: {
                c = getNextChar(B);
                dfaState = 49;
                break;
            }
            case 49:{
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                Value* val = (Value*)malloc(sizeof(Value));
                val->FLOAT_VALUE = stringToFloat(lex);
                createToken(token,TK_RNUM,lex,currentLine,2,val);
                acceptLexeme(B);
                return token;
                break;
            }
            case 32: {
                c = getNextChar(B);
                if(rangeMatch(c,'a','z') || rangeMatch(c,'A','Z')) {
                    dfaState = 33;
                }
                else {
                    // throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a function ID ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 33: {
                c = getNextChar(B);
                while(rangeMatch(c,'a','z') || rangeMatch(c,'A','Z'))
                    c = getNextChar(B);

                if(rangeMatch(c,'0','9')) {
                    dfaState = 34;
                }
                else {
                    dfaState = 35;
                }
                break;
            }
            case 34: {
                c = getNextChar(B);
                while(rangeMatch(c,'0','9'))
                    c = getNextChar(B);

                dfaState = 35;

                break;
            }
            case 35: {
                retractLexeme(B, 1);
                int identifierLength = B->lookahead - B->startLexeme + 1;
                if(identifierLength > 30) {
                    printf("Line %d : Function identifier length exceeds 30 characters\n" ,currentLine);
                    errorType = 4;
                    dfaState = 64;
                }
                else {
                    char* lex = copyString(B->startLexeme,B->lookahead);
                    if(c == '\n')
                        createToken(token,TK_FUNID,lex,currentLine,0,NULL);
                    else
                        createToken(token,TK_FUNID,lex,currentLine-1,0,NULL);
                    acceptLexeme(B);
                    return token;
                }
                break;
            }
            case 36: {
                c = getNextChar(B);
                if(rangeMatch(c,'a','z')) {
                    dfaState = 37;
                }
                else {
                    // Throw lexical error
                    char* pattern = copyString(B->startLexeme, B->lookahead-sizeof(char));
                    printf("Line %d : Cannot recognize pattern %s, Were you tring for a record ID ?\n" ,currentLine,pattern);
                    free(pattern);
                    errorType = 3;
                    dfaState = 64;

                    // Retract because an unforseen character lead the lexer to this dfaState, it can be a correct character which shouldl be included in the next token
                    retractLexeme(B, 1);
                }
                break;
            }
            case 37: {
                c = getNextChar( B);
                while(rangeMatch(c,'a','z'))
                    c = getNextChar( B);

                dfaState = 38;
                break;
            }
            case 38: {
                retractLexeme(B, 1);
                char* lex = copyString(B->startLexeme,B->lookahead);
                if(c == '\n')
                    createToken(token,TK_RUID,lex,currentLine-1,0,NULL);
                else
                    createToken(token,TK_RUID,lex,currentLine,0,NULL);
                acceptLexeme(B);
                return token;
                break;
            }
            case 64: {
                // Error State
                // Rationale 1 => Set a flag that error has reached for this input program so do not tokenize any further
                // Rationale 2 => Try to tokenize to the closest match and continue tokenizing further

                // Chosen Rationale => Panic mode, Travel up till a delimiter

                // Comment this, will bring it back to dfaState 0
                // c = getNextChar();
                // while(c != ';' && c !=  EOF && c != '\n') {
                //     c = getNextChar();
                // }

                char* lex = copyString(B->startLexeme,B->lookahead);

                // A retractLexemeiB, on only occurs if the errorType is 3, so check if the character read was a '\n'
                if(errorType == 3 && c == '\n')
                    createToken(token,TK_ERR,lex,currentLine-1,errorType,NULL);
                else
                    createToken(token,TK_ERR,lex,currentLine,errorType,NULL);
                acceptLexeme(B);

                // Move back to the start dfaState after setting B->startLexeme if an unidentifiable character causes the error
                return token;

                break;
            }
        }

}


}