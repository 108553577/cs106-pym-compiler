#ifndef PARSE_TREE_
#define PARSE_TREE_


#include <string>
#include <memory>
#include "token.h"

constexpr auto MAX_CHILDREN = 4;

// these are scoped enums, so no suffix needed
enum class NodeKind
{
	 STMT, EXPR, PARAM, LIST
};

enum class StmtKind
{
	DEF, DECL, CMPD, IFC,IF, WHILE, RETURN, EXPR, ELIF, ELSE
};

enum class ExprKind
{
	OP, NUM, STR, ID, CALL, ARRAY,ARRAYC
};

enum class ParamKind
{
	DYN, STC, STC_ARR
};

enum class ListKind
{
	STMT, PARAM, ARG, ELIF
};

enum class ExprType
{
	TBD, INT, NUM, STRING,
	
};

struct Rational
{
	int num, den; // numerator and denominator
	
	static Rational parse(const std::string& str);
	friend std::ostream& operator<<(std::ostream& os, const Rational& r);
};

bool checkMove(TokenType expected) {
    if (currentToken.type == expected) {
        currentToken = getNextToken();  // 假设 getNextToken 会更新 currentToken
        return true;
    } else {
        print("something wrong");
        *status = false;  // 如果匹配失败，设置 status 为 false
        return false;
    }
}

struct TreeNode
{
	std::shared_ptr<TreeNode> children[MAX_CHILDREN];
	std::shared_ptr<TreeNode> lSibling;
	std::shared_ptr<TreeNode> rSibling;

	int lineNo;

	NodeKind nodeKind;
	union Kind { StmtKind stmt; ExprKind expr; ParamKind param; ListKind list; } kind;
	union Attr{
		union ExprAttr{
			TokenType op;
			Rational num;
			char* str; // these pointers are owned by this pointer, and their space will be deallocated.
			char* id;
		} exprAttr;
		struct {
			ExprType type;
			bool isAddr;
			char* name;
			size_t size;
		} dclAttr;
	} attr;
	ExprType type;
	bool isAddr;

	void* something;

	TreeNode();

	~TreeNode();

	friend std::ostream& operator<<(std::ostream& os, const TreeNode& t);
};

std::shared_ptr<TreeNode> expression(Info* f, bool* status) {
    auto result = std::make_shared<TreeNode>();
    result->nodeKind = NodeKind::EXPR;
    bool s;

    // 尝试解析一个简单的表达式（这里简化为解析一个数值、ID 或操作符）
    if (checkMove(ID)) {  // 如果是标识符
        result->kind.expr = ExprKind::ID;
        result->attr.exprAttr.id = currentToken.value.c_str();  // 假设 currentToken.value 存储了标识符的名称
    } else if (checkMove(NUM)) {  // 如果是数字
        result->kind.expr = ExprKind::NUM;
        result->attr.exprAttr.num = Rational::parse(currentToken.value);  // 假设数字是以 Rational 类型解析
    } else if (checkMove(STRING)) {  // 如果是字符串
        result->kind.expr = ExprKind::STR;
        result->attr.exprAttr.str = currentToken.value.c_str();
    } else if (checkMove(OP)) {  // 如果是操作符
        result->kind.expr = ExprKind::OP;
        result->attr.exprAttr.op = currentToken.type;  // 假设操作符是通过 token 类型来标识
    } else {
        print("something wrong");
        *status = false;
        return nullptr;
    }

    *status = true;  // 如果解析成功，设置状态为 true
    return result;
}

std::shared_ptr<TreeNode> statement(Info* f, bool* status) {
    auto result = std::make_shared<TreeNode>();
    bool s;

    // 检查是否是 while 语句
    if (checkMove(While)) {
        result->nodeKind = NodeKind::STMT;
        result->kind.stmt = StmtKind::WHILE;
        result->children[0] = while_statement(f, status);  // 递归调用 while_statement 解析 while 语句
        if (*status == false) return nullptr;  // 如果解析失败，返回 nullptr
        return result;
    }

    // 检查是否是 if 语句
    if (checkMove(If)) {
        result->nodeKind = NodeKind::STMT;
        result->kind.stmt = StmtKind::IF;
        result->children[0] = expression(f, status);  // 解析 if 条件
        if (*status == false) return nullptr;  // 如果解析失败，返回 nullptr
        result->children[1] = statement(f, status);  // 解析 if 语句的主体
        if (*status == false) return nullptr;  // 如果解析失败，返回 nullptr
        return result;
    }

    // 检查是否是赋值语句（变量 = 表达式）
    if (checkMove(ID)) {
        result->nodeKind = NodeKind::STMT;
        result->kind.stmt = StmtKind::EXPR;
        result->children[0] = expression(f, status);  // 解析表达式
        if (*status == false) return nullptr;  // 如果解析失败，返回 nullptr
        return result;
    }

    // 检查是否是 return 语句
    if (checkMove(Return)) {
        result->nodeKind = NodeKind::STMT;
        result->kind.stmt = StmtKind::RETURN;
        result->children[0] = expression(f, status);  // 解析 return 表达式
        if (*status == false) return nullptr;  // 如果解析失败，返回 nullptr
        return result;
    }

    // 如果没有匹配任何已知语句类型，打印错误并返回 nullptr
    print("something wrong");
    *status = false;
    return nullptr;
}

std::shared_ptr<TreeNode> while_statement(Info* f, bool* status) {
    auto result = std::make_shared<TreeNode>();
    result->nodeKind = NodeKind::STMT;
    result->kind.stmt = StmtKind::WHILE;
    bool s;

    // Step 1: Check if the current token is 'while'
    if (checkMove(While)) {
        
        // Step 2: Check if the next token is '('
        if (checkMove(LPAR)) {
            
            // Step 3: Parse the expression inside the parentheses
            result->children[0] = expression(f, &s);  // Parse the condition expression
            if (s == false) {
                print("something wrong");  // Expression parsing failed
                *status = false;
                return nullptr;
            }

            // Step 4: Check if the next token is ')'
            if (checkMove(RPAR)) {
                
                // Step 5: Parse the statement following the 'while' condition
                result->children[1] = statement(f, &s);  // Parse the statement
                if (s == false) {
                    print("something wrong");  // Statement parsing failed
                    *status = false;
                    return nullptr;
                }

                *status = true;  // Successfully parsed the 'while' statement
                return result;
            } else {
                print("something wrong");  // Expected closing parenthesis ')'
                *status = false;
                return nullptr;
            }
        } else {
            print("something wrong");  // Expected opening parenthesis '('
            *status = false;
            return nullptr;
        }
    }

    // If not 'while', print error and return nullptr
    print("something wrong");
    *status = false;
    return nullptr;
}


std::shared_ptr<TreeNode> if_statement(Info* f, bool* status) {
    auto result = std::make_shared<TreeNode>();
    result->nodeKind = NodeKind::STMT;
    result->kind.stmt = StmtKind::IF;
    bool s;

    // Step 1: Check if the current token is 'if'
    if (checkMove(If)) {
        
        // Step 2: Check if the next token is '('
        if (checkMove(LPAR)) {
            
            // Step 3: Parse the expression inside the parentheses
            result->children[0] = expression(f, &s);  // Parse the condition expression
            if (s == false) {
                return nullptr;  // If expression parsing failed, return nullptr
            }
            
            // Step 4: Check if the next token is ')'
            if (checkMove(RPAR)) {
                
                // Step 5: Parse the statement following the 'if' condition
                result->children[1] = statement(f, &s);  // Parse the statement
                if (s == false) {
                    return nullptr;  // If statement parsing failed, return nullptr
                }

                *status = true;  // Successfully parsed the 'if' statement
                return result;
            } else {
                print("something wrong");  // Expected closing parenthesis ')'
                *status = false;
                return nullptr;
            }
        } else {
            print("something wrong");  // Expected opening parenthesis '('
            *status = false;
            return nullptr;
        }
    }

    // If not 'if', print error and return nullptr
    print("something wrong");
    *status = false;
    return nullptr;
}

std::shared_ptr<TreeNode> switch_statement(Info* f, bool* status) {
    auto result = std::make_shared<TreeNode>();
    result->nodeKind = NodeKind::STMT;
    result->kind.stmt = StmtKind::DEF;  // Using StmtKind::DEF for switch
    bool s;

    // Step 1: Check if the current token is 'switch'
    if (checkMove(Switch)) {

        // Step 2: Check if the next token is '('
        if (checkMove(LPAR)) {

            // Step 3: Parse the expression inside the parentheses
            result->children[0] = expression(f, &s);  // Parse the condition expression
            if (s == false) {
                print("something wrong");  // Expression parsing failed
                *status = false;
                return nullptr;
            }

            // Step 4: Check if the next token is ')'
            if (checkMove(RPAR)) {

                // Step 5: Check if the next token is '{', indicating the start of cases
                if (checkMove(LBRACE)) {

                    // Step 6: Parse the case statements
                    while (true) {
                        // Check for the next case or default
                        if (checkMove(Case)) {
                            // Parse the case expression
                            auto caseNode = std::make_shared<TreeNode>();
                            caseNode->nodeKind = NodeKind::STMT;
                            caseNode->kind.stmt = StmtKind::CASE;
                            
                            // Parse the case condition expression
                            caseNode->children[0] = expression(f, &s);
                            if (s == false) {
                                print("something wrong");  // Case expression parsing failed
                                *status = false;
                                return nullptr;
                            }

                            // Parse the case statement
                            caseNode->children[1] = statement(f, &s);
                            if (s == false) {
                                print("something wrong");  // Case statement parsing failed
                                *status = false;
                                return nullptr;
                            }

                            // Add the parsed case to the children of the result
                            result->children[result->children.size()] = caseNode;
                        }
                        // Check for the 'default' case
                        else if (checkMove(Default)) {
                            auto defaultNode = std::make_shared<TreeNode>();
                            defaultNode->nodeKind = NodeKind::STMT;
                            defaultNode->kind.stmt = StmtKind::DEFAULT;
                            
                            // Parse the default statement
                            defaultNode->children[0] = statement(f, &s);
                            if (s == false) {
                                print("something wrong");  // Default statement parsing failed
                                *status = false;
                                return nullptr;
                            }

                            // Add the parsed default to the children of the result
                            result->children[result->children.size()] = defaultNode;
                            break;  // Break out of the loop as default ends the switch
                        } else {
                            break;  // Break if there are no more case or default
                        }
                    }

                    // Step 7: Successfully parsed the switch statement
                    *status = true;
                    return result;
                } else {
                    print("something wrong");  // Expected opening brace '{' for switch cases
                    *status = false;
                    return nullptr;
                }
            } else {
                print("something wrong");  // Expected closing parenthesis ')'
                *status = false;
                return nullptr;
            }
        } else {
            print("something wrong");  // Expected opening parenthesis '('
            *status = false;
            return nullptr;
        }
    }

    // If not 'switch', print error and return nullptr
    print("something wrong");
    *status = false;
    return nullptr;
}

std::shared_ptr<TreeNode> for_statement(Info* f, bool* status) {
    auto result = std::make_shared<TreeNode>();
    result->nodeKind = NodeKind::STMT;
    result->kind.stmt = StmtKind::WHILE;  // For consistency, using StmtKind::WHILE
    bool s;

    // Step 1: Check if the current token is 'for'
    if (checkMove(For)) {

        // Step 2: Check if the next token is '('
        if (checkMove(LPAR)) {

            // Step 3: Parse the initialization expression (before the first semicolon)
            result->children[0] = statement(f, &s);  // Usually an initialization statement
            if (s == false) {
                print("something wrong");  // Initialization parsing failed
                *status = false;
                return nullptr;
            }

            // Step 4: Parse the condition expression (between the semicolons)
            result->children[1] = expression(f, &s);  // Loop condition expression
            if (s == false) {
                print("something wrong");  // Condition parsing failed
                *status = false;
                return nullptr;
            }

            // Step 5: Parse the increment expression (after the second semicolon)
            result->children[2] = expression(f, &s);  // Increment expression
            if (s == false) {
                print("something wrong");  // Increment parsing failed
                *status = false;
                return nullptr;
            }

            // Step 6: Check if the next token is ')'
            if (checkMove(RPAR)) {

                // Step 7: Parse the statement inside the for loop
                result->children[3] = statement(f, &s);  // Body of the for loop
                if (s == false) {
                    print("something wrong");  // Statement inside the loop parsing failed
                    *status = false;
                    return nullptr;
                }

                // Step 8: Successfully parsed the for statement
                *status = true;
                return result;
            } else {
                print("something wrong");  // Expected closing parenthesis ')'
                *status = false;
                return nullptr;
            }
        } else {
            print("something wrong");  // Expected opening parenthesis '('
            *status = false;
            return nullptr;
        }
    }

    // If not 'for', print error and return nullptr
    print("something wrong");
    *status = false;
    return nullptr;
}



#endif
