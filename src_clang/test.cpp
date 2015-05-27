#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

using namespace clang;
using namespace clang::tooling;
using namespace std;

struct Field {
    string type_;
    string name_;

    Field(string type, string name) : type_(type), name_(name) {}

    std::string getAsString() const { return type_ + " " + name_; }
};

struct Proto {
    string name;
    vector<Field> fields;

    std::string getDefinition() const {
        std::ostringstream out;
        out << "Message " << name << " {" << endl;
        int sequence = 1;
        for (const auto &field : fields) {
            out << "optional " << field.getAsString() << " = " << sequence++
                << ";" << endl;
        }
        out << "}" << endl;
        return out.str();
    }
};
static llvm::cl::OptionCategory ToolingSampleCategory("test sapmle");
class FindNamedClassVisitor
    : public RecursiveASTVisitor<FindNamedClassVisitor> {
   public:
    explicit FindNamedClassVisitor(ASTContext *Context) : Context(Context) {}

    bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
        Proto message;
        // Get name of the class.
        if (Declaration->getQualifiedNameAsString().find("anonymous") !=
            std::string::npos) {
            // anonymous class
            const Decl *nextDecl = Declaration->getNextDeclInContext();
            if (nextDecl && nextDecl->getKind() == Decl::Typedef) {
                const TypedefDecl *typedefDecl =
                    (const TypedefDecl *)(nextDecl);
                message.name = typedefDecl->getNameAsString();
            } else {
                cout << "some error might happen." << endl;
            }

        } else {
            message.name = Declaration->getQualifiedNameAsString();
        }
        // Get fields of the class.
        for (const auto &fieldIterator : Declaration->fields()) {
            message.fields.push_back(getFieldFrom(fieldIterator));
        }
        if (message.name.find("yfit") != std::string::npos) {
            cout << message.getDefinition();
        }
        return true;
    }

    Field getFieldFrom(const FieldDecl *const fieldIterator) {
        string type("null");
        if (isa<TypedefType>(fieldIterator->getType().getTypePtr())) {
            const BuiltinType *t =
                fieldIterator->getType()->getAs<BuiltinType>();
            if (t) {
                type = QualType(t, 0).getAsString();
            }
        } else {
            type = fieldIterator->getType().getAsString();
        }
        string name = fieldIterator->getNameAsString();
        return Field(type, name);
    }

   private:
    ASTContext *Context;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
   public:
    explicit FindNamedClassConsumer(ASTContext *Context) : Visitor(Context) {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

   private:
    FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
   public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::unique_ptr<clang::ASTConsumer>(
            new FindNamedClassConsumer(&Compiler.getASTContext()));
    }
};

int main(int argc, const char **argv) {
    CommonOptionsParser op(argc, argv, ToolingSampleCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
    return Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());
}
