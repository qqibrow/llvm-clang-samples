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

    std::string getAsString() const { return  type_ + " " + name_; }
};

struct Proto {
    string name;
    vector<Field> fields;

    std::string getDefinition() const {
         std::ostringstream out;
        out << "Message " << name << "{" << endl;
        for (const auto &field : fields) {
            out << field.getAsString() << endl;
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
        if (Declaration->getQualifiedNameAsString().find("anonymous") !=
            std::string::npos) {
            // anonymous class
            for (const auto &fieldIterator : Declaration->fields()) {
                string type = fieldIterator->getType().getAsString();
                string name = fieldIterator->getNameAsString();
                message.fields.push_back(Field(type, name));
            }
            const Decl *nextDecl = Declaration->getNextDeclInContext();
            if (nextDecl && nextDecl->getKind() == Decl::Typedef) {
                const TypedefDecl *typedefDecl =
                    (const TypedefDecl *)(nextDecl);
                message.name = typedefDecl->getNameAsString();
                cout << message.getDefinition();
            } else {
                cout << "some error might happen." << endl;
            }

        } else {
            message.name = Declaration->getQualifiedNameAsString();
            for (const auto &fieldIterator : Declaration->fields()) {
                string type = fieldIterator->getType().getAsString();
                string name = fieldIterator->getNameAsString();
                message.fields.push_back(Field(type, name));
            }
            cout << message.getDefinition();
        }
        Declaration->dump();

        /*
        for(auto i = Declaration->decls_begin(); i != Declaration->decls_end();
        ++i) {
            cout << i->getDeclKindName();
        }
        */
        /*
  if (Declaration->getQualifiedNameAsString() == "n::m::C") {
      Declaration->
    FullSourceLoc FullLocation =
  Context->getFullLoc(Declaration->getLocStart());
    if (FullLocation.isValid())
      llvm::outs() << "Found declaration at "
                   << FullLocation.getSpellingLineNumber() << ":"
                   << FullLocation.getSpellingColumnNumber() << "\n";
  }
  */
        return true;
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
