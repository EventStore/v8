// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_AST_H_
#define V8_AST_H_

#include "src/v8.h"

#include "src/assembler.h"
#include "src/factory.h"
#include "src/feedback-slots.h"
#include "src/isolate.h"
#include "src/jsregexp.h"
#include "src/list-inl.h"
#include "src/runtime.h"
#include "src/small-pointer-list.h"
#include "src/smart-pointers.h"
#include "src/token.h"
#include "src/types.h"
#include "src/utils.h"
#include "src/variables.h"
#include "src/interface.h"
#include "src/zone-inl.h"

namespace v8 {
namespace internal {

// The abstract syntax tree is an intermediate, light-weight
// representation of the parsed JavaScript code suitable for
// compilation to native code.

// Nodes are allocated in a separate zone, which allows faster
// allocation and constant-time deallocation of the entire syntax
// tree.


// ----------------------------------------------------------------------------
// Nodes of the abstract syntax tree. Only concrete classes are
// enumerated here.

#define DECLARATION_NODE_LIST(V)                \
  V(VariableDeclaration)                        \
  V(FunctionDeclaration)                        \
  V(ModuleDeclaration)                          \
  V(ImportDeclaration)                          \
  V(ExportDeclaration)                          \

#define MODULE_NODE_LIST(V)                     \
  V(ModuleLiteral)                              \
  V(ModuleVariable)                             \
  V(ModulePath)                                 \
  V(ModuleUrl)

#define STATEMENT_NODE_LIST(V)                  \
  V(Block)                                      \
  V(ModuleStatement)                            \
  V(ExpressionStatement)                        \
  V(EmptyStatement)                             \
  V(IfStatement)                                \
  V(ContinueStatement)                          \
  V(BreakStatement)                             \
  V(ReturnStatement)                            \
  V(WithStatement)                              \
  V(SwitchStatement)                            \
  V(DoWhileStatement)                           \
  V(WhileStatement)                             \
  V(ForStatement)                               \
  V(ForInStatement)                             \
  V(ForOfStatement)                             \
  V(TryCatchStatement)                          \
  V(TryFinallyStatement)                        \
  V(DebuggerStatement)

#define EXPRESSION_NODE_LIST(V)                 \
  V(FunctionLiteral)                            \
  V(NativeFunctionLiteral)                      \
  V(Conditional)                                \
  V(VariableProxy)                              \
  V(Literal)                                    \
  V(RegExpLiteral)                              \
  V(ObjectLiteral)                              \
  V(ArrayLiteral)                               \
  V(Assignment)                                 \
  V(Yield)                                      \
  V(Throw)                                      \
  V(Property)                                   \
  V(Call)                                       \
  V(CallNew)                                    \
  V(CallRuntime)                                \
  V(UnaryOperation)                             \
  V(CountOperation)                             \
  V(BinaryOperation)                            \
  V(CompareOperation)                           \
  V(ThisFunction)                               \
  V(CaseClause)

#define AST_NODE_LIST(V)                        \
  DECLARATION_NODE_LIST(V)                      \
  MODULE_NODE_LIST(V)                           \
  STATEMENT_NODE_LIST(V)                        \
  EXPRESSION_NODE_LIST(V)

// Forward declarations
class AstConstructionVisitor;
template<class> class AstNodeFactory;
class AstVisitor;
class Declaration;
class Module;
class BreakableStatement;
class Expression;
class IterationStatement;
class MaterializedLiteral;
class Statement;
class TargetCollector;
class TypeFeedbackOracle;

class RegExpAlternative;
class RegExpAssertion;
class RegExpAtom;
class RegExpBackReference;
class RegExpCapture;
class RegExpCharacterClass;
class RegExpCompiler;
class RegExpDisjunction;
class RegExpEmpty;
class RegExpLookahead;
class RegExpQuantifier;
class RegExpText;

#define DEF_FORWARD_DECLARATION(type) class type;
AST_NODE_LIST(DEF_FORWARD_DECLARATION)
#undef DEF_FORWARD_DECLARATION


// Typedef only introduced to avoid unreadable code.
// Please do appreciate the required space in "> >".
typedef ZoneList<Handle<String> > ZoneStringList;
typedef ZoneList<Handle<Object> > ZoneObjectList;


#define DECLARE_NODE_TYPE(type)                                 \
  virtual void Accept(AstVisitor* v) V8_OVERRIDE;                  \
  virtual AstNode::NodeType node_type() const V8_FINAL V8_OVERRIDE {  \
    return AstNode::k##type;                                    \
  }                                                             \
  template<class> friend class AstNodeFactory;


enum AstPropertiesFlag {
  kDontInline,
  kDontSelfOptimize,
  kDontSoftInline,
  kDontCache
};


class AstProperties V8_FINAL BASE_EMBEDDED {
 public:
  class Flags : public EnumSet<AstPropertiesFlag, int> {};

AstProperties() : node_count_(0), feedback_slots_(0) {}

  Flags* flags() { return &flags_; }
  int node_count() { return node_count_; }
  void add_node_count(int count) { node_count_ += count; }

  int feedback_slots() const { return feedback_slots_; }
  void increase_feedback_slots(int count) {
    feedback_slots_ += count;
  }

 private:
  Flags flags_;
  int node_count_;
  int feedback_slots_;
};


class AstNode: public ZoneObject {
 public:
#define DECLARE_TYPE_ENUM(type) k##type,
  enum NodeType {
    AST_NODE_LIST(DECLARE_TYPE_ENUM)
    kInvalid = -1
  };
#undef DECLARE_TYPE_ENUM

  void* operator new(size_t size, Zone* zone) {
    return zone->New(static_cast<int>(size));
  }

  explicit AstNode(int position): position_(position) {}
  virtual ~AstNode() {}

  virtual void Accept(AstVisitor* v) = 0;
  virtual NodeType node_type() const = 0;
  int position() const { return position_; }

  // Type testing & conversion functions overridden by concrete subclasses.
#define DECLARE_NODE_FUNCTIONS(type) \
  bool Is##type() const { return node_type() == AstNode::k##type; } \
  type* As##type() { \
    return Is##type() ? reinterpret_cast<type*>(this) : NULL; \
  } \
  const type* As##type() const { \
    return Is##type() ? reinterpret_cast<const type*>(this) : NULL; \
  }
  AST_NODE_LIST(DECLARE_NODE_FUNCTIONS)
#undef DECLARE_NODE_FUNCTIONS

  virtual TargetCollector* AsTargetCollector() { return NULL; }
  virtual BreakableStatement* AsBreakableStatement() { return NULL; }
  virtual IterationStatement* AsIterationStatement() { return NULL; }
  virtual MaterializedLiteral* AsMaterializedLiteral() { return NULL; }

 protected:
  static int GetNextId(Zone* zone) {
    return ReserveIdRange(zone, 1);
  }

  static int ReserveIdRange(Zone* zone, int n) {
    int tmp = zone->isolate()->ast_node_id();
    zone->isolate()->set_ast_node_id(tmp + n);
    return tmp;
  }

  // Some nodes re-use bailout IDs for type feedback.
  static TypeFeedbackId reuse(BailoutId id) {
    return TypeFeedbackId(id.ToInt());
  }


 private:
  // Hidden to prevent accidental usage. It would have to load the
  // current zone from the TLS.
  void* operator new(size_t size);

  friend class CaseClause;  // Generates AST IDs.

  int position_;
};


class Statement : public AstNode {
 public:
  explicit Statement(Zone* zone, int position) : AstNode(position) {}

  bool IsEmpty() { return AsEmptyStatement() != NULL; }
  virtual bool IsJump() const { return false; }
};


class SmallMapList V8_FINAL {
 public:
  SmallMapList() {}
  SmallMapList(int capacity, Zone* zone) : list_(capacity, zone) {}

  void Reserve(int capacity, Zone* zone) { list_.Reserve(capacity, zone); }
  void Clear() { list_.Clear(); }
  void Sort() { list_.Sort(); }

  bool is_empty() const { return list_.is_empty(); }
  int length() const { return list_.length(); }

  void AddMapIfMissing(Handle<Map> map, Zone* zone) {
    if (!Map::CurrentMapForDeprecated(map).ToHandle(&map)) return;
    for (int i = 0; i < length(); ++i) {
      if (at(i).is_identical_to(map)) return;
    }
    Add(map, zone);
  }

  void FilterForPossibleTransitions(Map* root_map) {
    for (int i = list_.length() - 1; i >= 0; i--) {
      if (at(i)->FindRootMap() != root_map) {
        list_.RemoveElement(list_.at(i));
      }
    }
  }

  void Add(Handle<Map> handle, Zone* zone) {
    list_.Add(handle.location(), zone);
  }

  Handle<Map> at(int i) const {
    return Handle<Map>(list_.at(i));
  }

  Handle<Map> first() const { return at(0); }
  Handle<Map> last() const { return at(length() - 1); }

 private:
  // The list stores pointers to Map*, that is Map**, so it's GC safe.
  SmallPointerList<Map*> list_;

  DISALLOW_COPY_AND_ASSIGN(SmallMapList);
};


class Expression : public AstNode {
 public:
  enum Context {
    // Not assigned a context yet, or else will not be visited during
    // code generation.
    kUninitialized,
    // Evaluated for its side effects.
    kEffect,
    // Evaluated for its value (and side effects).
    kValue,
    // Evaluated for control flow (and side effects).
    kTest
  };

  virtual bool IsValidReferenceExpression() const { return false; }

  // Helpers for ToBoolean conversion.
  virtual bool ToBooleanIsTrue() const { return false; }
  virtual bool ToBooleanIsFalse() const { return false; }

  // Symbols that cannot be parsed as array indices are considered property
  // names.  We do not treat symbols that can be array indexes as property
  // names because [] for string objects is handled only by keyed ICs.
  virtual bool IsPropertyName() const { return false; }

  // True iff the result can be safely overwritten (to avoid allocation).
  // False for operations that can return one of their operands.
  virtual bool ResultOverwriteAllowed() const { return false; }

  // True iff the expression is a literal represented as a smi.
  bool IsSmiLiteral() const;

  // True iff the expression is a string literal.
  bool IsStringLiteral() const;

  // True iff the expression is the null literal.
  bool IsNullLiteral() const;

  // True if we can prove that the expression is the undefined literal.
  bool IsUndefinedLiteral(Isolate* isolate) const;

  // Expression type bounds
  Bounds bounds() const { return bounds_; }
  void set_bounds(Bounds bounds) { bounds_ = bounds; }

  // Type feedback information for assignments and properties.
  virtual bool IsMonomorphic() {
    UNREACHABLE();
    return false;
  }
  virtual SmallMapList* GetReceiverTypes() {
    UNREACHABLE();
    return NULL;
  }
  virtual KeyedAccessStoreMode GetStoreMode() {
    UNREACHABLE();
    return STANDARD_STORE;
  }

  // TODO(rossberg): this should move to its own AST node eventually.
  virtual void RecordToBooleanTypeFeedback(TypeFeedbackOracle* oracle);
  byte to_boolean_types() const { return to_boolean_types_; }

  BailoutId id() const { return id_; }
  TypeFeedbackId test_id() const { return test_id_; }

 protected:
  Expression(Zone* zone, int pos)
      : AstNode(pos),
        bounds_(Bounds::Unbounded(zone)),
        id_(GetNextId(zone)),
        test_id_(GetNextId(zone)) {}
  void set_to_boolean_types(byte types) { to_boolean_types_ = types; }

 private:
  Bounds bounds_;
  byte to_boolean_types_;

  const BailoutId id_;
  const TypeFeedbackId test_id_;
};


class BreakableStatement : public Statement {
 public:
  enum BreakableType {
    TARGET_FOR_ANONYMOUS,
    TARGET_FOR_NAMED_ONLY
  };

  // The labels associated with this statement. May be NULL;
  // if it is != NULL, guaranteed to contain at least one entry.
  ZoneStringList* labels() const { return labels_; }

  // Type testing & conversion.
  virtual BreakableStatement* AsBreakableStatement() V8_FINAL V8_OVERRIDE {
    return this;
  }

  // Code generation
  Label* break_target() { return &break_target_; }

  // Testers.
  bool is_target_for_anonymous() const {
    return breakable_type_ == TARGET_FOR_ANONYMOUS;
  }

  BailoutId EntryId() const { return entry_id_; }
  BailoutId ExitId() const { return exit_id_; }

 protected:
  BreakableStatement(
      Zone* zone, ZoneStringList* labels,
      BreakableType breakable_type, int position)
      : Statement(zone, position),
        labels_(labels),
        breakable_type_(breakable_type),
        entry_id_(GetNextId(zone)),
        exit_id_(GetNextId(zone)) {
    ASSERT(labels == NULL || labels->length() > 0);
  }


 private:
  ZoneStringList* labels_;
  BreakableType breakable_type_;
  Label break_target_;
  const BailoutId entry_id_;
  const BailoutId exit_id_;
};


class Block V8_FINAL : public BreakableStatement {
 public:
  DECLARE_NODE_TYPE(Block)

  void AddStatement(Statement* statement, Zone* zone) {
    statements_.Add(statement, zone);
  }

  ZoneList<Statement*>* statements() { return &statements_; }
  bool is_initializer_block() const { return is_initializer_block_; }

  BailoutId DeclsId() const { return decls_id_; }

  virtual bool IsJump() const V8_OVERRIDE {
    return !statements_.is_empty() && statements_.last()->IsJump()
        && labels() == NULL;  // Good enough as an approximation...
  }

  Scope* scope() const { return scope_; }
  void set_scope(Scope* scope) { scope_ = scope; }

 protected:
  Block(Zone* zone,
        ZoneStringList* labels,
        int capacity,
        bool is_initializer_block,
        int pos)
      : BreakableStatement(zone, labels, TARGET_FOR_NAMED_ONLY, pos),
        statements_(capacity, zone),
        is_initializer_block_(is_initializer_block),
        decls_id_(GetNextId(zone)),
        scope_(NULL) {
  }

 private:
  ZoneList<Statement*> statements_;
  bool is_initializer_block_;
  const BailoutId decls_id_;
  Scope* scope_;
};


class Declaration : public AstNode {
 public:
  VariableProxy* proxy() const { return proxy_; }
  VariableMode mode() const { return mode_; }
  Scope* scope() const { return scope_; }
  virtual InitializationFlag initialization() const = 0;
  virtual bool IsInlineable() const;

 protected:
  Declaration(Zone* zone,
              VariableProxy* proxy,
              VariableMode mode,
              Scope* scope,
              int pos)
      : AstNode(pos),
        proxy_(proxy),
        mode_(mode),
        scope_(scope) {
    ASSERT(IsDeclaredVariableMode(mode));
  }

 private:
  VariableProxy* proxy_;
  VariableMode mode_;

  // Nested scope from which the declaration originated.
  Scope* scope_;
};


class VariableDeclaration V8_FINAL : public Declaration {
 public:
  DECLARE_NODE_TYPE(VariableDeclaration)

  virtual InitializationFlag initialization() const V8_OVERRIDE {
    return mode() == VAR ? kCreatedInitialized : kNeedsInitialization;
  }

 protected:
  VariableDeclaration(Zone* zone,
                      VariableProxy* proxy,
                      VariableMode mode,
                      Scope* scope,
                      int pos)
      : Declaration(zone, proxy, mode, scope, pos) {
  }
};


class FunctionDeclaration V8_FINAL : public Declaration {
 public:
  DECLARE_NODE_TYPE(FunctionDeclaration)

  FunctionLiteral* fun() const { return fun_; }
  virtual InitializationFlag initialization() const V8_OVERRIDE {
    return kCreatedInitialized;
  }
  virtual bool IsInlineable() const V8_OVERRIDE;

 protected:
  FunctionDeclaration(Zone* zone,
                      VariableProxy* proxy,
                      VariableMode mode,
                      FunctionLiteral* fun,
                      Scope* scope,
                      int pos)
      : Declaration(zone, proxy, mode, scope, pos),
        fun_(fun) {
    // At the moment there are no "const functions" in JavaScript...
    ASSERT(mode == VAR || mode == LET);
    ASSERT(fun != NULL);
  }

 private:
  FunctionLiteral* fun_;
};


class ModuleDeclaration V8_FINAL : public Declaration {
 public:
  DECLARE_NODE_TYPE(ModuleDeclaration)

  Module* module() const { return module_; }
  virtual InitializationFlag initialization() const V8_OVERRIDE {
    return kCreatedInitialized;
  }

 protected:
  ModuleDeclaration(Zone* zone,
                    VariableProxy* proxy,
                    Module* module,
                    Scope* scope,
                    int pos)
      : Declaration(zone, proxy, MODULE, scope, pos),
        module_(module) {
  }

 private:
  Module* module_;
};


class ImportDeclaration V8_FINAL : public Declaration {
 public:
  DECLARE_NODE_TYPE(ImportDeclaration)

  Module* module() const { return module_; }
  virtual InitializationFlag initialization() const V8_OVERRIDE {
    return kCreatedInitialized;
  }

 protected:
  ImportDeclaration(Zone* zone,
                    VariableProxy* proxy,
                    Module* module,
                    Scope* scope,
                    int pos)
      : Declaration(zone, proxy, LET, scope, pos),
        module_(module) {
  }

 private:
  Module* module_;
};


class ExportDeclaration V8_FINAL : public Declaration {
 public:
  DECLARE_NODE_TYPE(ExportDeclaration)

  virtual InitializationFlag initialization() const V8_OVERRIDE {
    return kCreatedInitialized;
  }

 protected:
  ExportDeclaration(Zone* zone, VariableProxy* proxy, Scope* scope, int pos)
      : Declaration(zone, proxy, LET, scope, pos) {}
};


class Module : public AstNode {
 public:
  Interface* interface() const { return interface_; }
  Block* body() const { return body_; }

 protected:
  Module(Zone* zone, int pos)
      : AstNode(pos),
        interface_(Interface::NewModule(zone)),
        body_(NULL) {}
  Module(Zone* zone, Interface* interface, int pos, Block* body = NULL)
      : AstNode(pos),
        interface_(interface),
        body_(body) {}

 private:
  Interface* interface_;
  Block* body_;
};


class ModuleLiteral V8_FINAL : public Module {
 public:
  DECLARE_NODE_TYPE(ModuleLiteral)

 protected:
  ModuleLiteral(Zone* zone, Block* body, Interface* interface, int pos)
      : Module(zone, interface, pos, body) {}
};


class ModuleVariable V8_FINAL : public Module {
 public:
  DECLARE_NODE_TYPE(ModuleVariable)

  VariableProxy* proxy() const { return proxy_; }

 protected:
  inline ModuleVariable(Zone* zone, VariableProxy* proxy, int pos);

 private:
  VariableProxy* proxy_;
};


class ModulePath V8_FINAL : public Module {
 public:
  DECLARE_NODE_TYPE(ModulePath)

  Module* module() const { return module_; }
  Handle<String> name() const { return name_; }

 protected:
  ModulePath(Zone* zone, Module* module, Handle<String> name, int pos)
      : Module(zone, pos),
        module_(module),
        name_(name) {
  }

 private:
  Module* module_;
  Handle<String> name_;
};


class ModuleUrl V8_FINAL : public Module {
 public:
  DECLARE_NODE_TYPE(ModuleUrl)

  Handle<String> url() const { return url_; }

 protected:
  ModuleUrl(Zone* zone, Handle<String> url, int pos)
      : Module(zone, pos), url_(url) {
  }

 private:
  Handle<String> url_;
};


class ModuleStatement V8_FINAL : public Statement {
 public:
  DECLARE_NODE_TYPE(ModuleStatement)

  VariableProxy* proxy() const { return proxy_; }
  Block* body() const { return body_; }

 protected:
  ModuleStatement(Zone* zone, VariableProxy* proxy, Block* body, int pos)
      : Statement(zone, pos),
        proxy_(proxy),
        body_(body) {
  }

 private:
  VariableProxy* proxy_;
  Block* body_;
};


class IterationStatement : public BreakableStatement {
 public:
  // Type testing & conversion.
  virtual IterationStatement* AsIterationStatement() V8_FINAL V8_OVERRIDE {
    return this;
  }

  Statement* body() const { return body_; }

  BailoutId OsrEntryId() const { return osr_entry_id_; }
  virtual BailoutId ContinueId() const = 0;
  virtual BailoutId StackCheckId() const = 0;

  // Code generation
  Label* continue_target()  { return &continue_target_; }

 protected:
  IterationStatement(Zone* zone, ZoneStringList* labels, int pos)
      : BreakableStatement(zone, labels, TARGET_FOR_ANONYMOUS, pos),
        body_(NULL),
        osr_entry_id_(GetNextId(zone)) {
  }

  void Initialize(Statement* body) {
    body_ = body;
  }

 private:
  Statement* body_;
  Label continue_target_;

  const BailoutId osr_entry_id_;
};


class DoWhileStatement V8_FINAL : public IterationStatement {
 public:
  DECLARE_NODE_TYPE(DoWhileStatement)

  void Initialize(Expression* cond, Statement* body) {
    IterationStatement::Initialize(body);
    cond_ = cond;
  }

  Expression* cond() const { return cond_; }

  virtual BailoutId ContinueId() const V8_OVERRIDE { return continue_id_; }
  virtual BailoutId StackCheckId() const V8_OVERRIDE { return back_edge_id_; }
  BailoutId BackEdgeId() const { return back_edge_id_; }

 protected:
  DoWhileStatement(Zone* zone, ZoneStringList* labels, int pos)
      : IterationStatement(zone, labels, pos),
        cond_(NULL),
        continue_id_(GetNextId(zone)),
        back_edge_id_(GetNextId(zone)) {
  }

 private:
  Expression* cond_;

  const BailoutId continue_id_;
  const BailoutId back_edge_id_;
};


class WhileStatement V8_FINAL : public IterationStatement {
 public:
  DECLARE_NODE_TYPE(WhileStatement)

  void Initialize(Expression* cond, Statement* body) {
    IterationStatement::Initialize(body);
    cond_ = cond;
  }

  Expression* cond() const { return cond_; }
  bool may_have_function_literal() const {
    return may_have_function_literal_;
  }
  void set_may_have_function_literal(bool value) {
    may_have_function_literal_ = value;
  }

  virtual BailoutId ContinueId() const V8_OVERRIDE { return EntryId(); }
  virtual BailoutId StackCheckId() const V8_OVERRIDE { return body_id_; }
  BailoutId BodyId() const { return body_id_; }

 protected:
  WhileStatement(Zone* zone, ZoneStringList* labels, int pos)
      : IterationStatement(zone, labels, pos),
        cond_(NULL),
        may_have_function_literal_(true),
        body_id_(GetNextId(zone)) {
  }

 private:
  Expression* cond_;

  // True if there is a function literal subexpression in the condition.
  bool may_have_function_literal_;

  const BailoutId body_id_;
};


class ForStatement V8_FINAL : public IterationStatement {
 public:
  DECLARE_NODE_TYPE(ForStatement)

  void Initialize(Statement* init,
                  Expression* cond,
                  Statement* next,
                  Statement* body) {
    IterationStatement::Initialize(body);
    init_ = init;
    cond_ = cond;
    next_ = next;
  }

  Statement* init() const { return init_; }
  Expression* cond() const { return cond_; }
  Statement* next() const { return next_; }

  bool may_have_function_literal() const {
    return may_have_function_literal_;
  }
  void set_may_have_function_literal(bool value) {
    may_have_function_literal_ = value;
  }

  virtual BailoutId ContinueId() const V8_OVERRIDE { return continue_id_; }
  virtual BailoutId StackCheckId() const V8_OVERRIDE { return body_id_; }
  BailoutId BodyId() const { return body_id_; }

  bool is_fast_smi_loop() { return loop_variable_ != NULL; }
  Variable* loop_variable() { return loop_variable_; }
  void set_loop_variable(Variable* var) { loop_variable_ = var; }

 protected:
  ForStatement(Zone* zone, ZoneStringList* labels, int pos)
      : IterationStatement(zone, labels, pos),
        init_(NULL),
        cond_(NULL),
        next_(NULL),
        may_have_function_literal_(true),
        loop_variable_(NULL),
        continue_id_(GetNextId(zone)),
        body_id_(GetNextId(zone)) {
  }

 private:
  Statement* init_;
  Expression* cond_;
  Statement* next_;

  // True if there is a function literal subexpression in the condition.
  bool may_have_function_literal_;
  Variable* loop_variable_;

  const BailoutId continue_id_;
  const BailoutId body_id_;
};


class ForEachStatement : public IterationStatement {
 public:
  enum VisitMode {
    ENUMERATE,   // for (each in subject) body;
    ITERATE      // for (each of subject) body;
  };

  void Initialize(Expression* each, Expression* subject, Statement* body) {
    IterationStatement::Initialize(body);
    each_ = each;
    subject_ = subject;
  }

  Expression* each() const { return each_; }
  Expression* subject() const { return subject_; }

 protected:
  ForEachStatement(Zone* zone, ZoneStringList* labels, int pos)
      : IterationStatement(zone, labels, pos),
        each_(NULL),
        subject_(NULL) {
  }

 private:
  Expression* each_;
  Expression* subject_;
};


class ForInStatement V8_FINAL : public ForEachStatement,
    public FeedbackSlotInterface {
 public:
  DECLARE_NODE_TYPE(ForInStatement)

  Expression* enumerable() const {
    return subject();
  }

  // Type feedback information.
  virtual int ComputeFeedbackSlotCount() { return 1; }
  virtual void SetFirstFeedbackSlot(int slot) { for_in_feedback_slot_ = slot; }

  int ForInFeedbackSlot() {
    ASSERT(for_in_feedback_slot_ != kInvalidFeedbackSlot);
    return for_in_feedback_slot_;
  }

  enum ForInType { FAST_FOR_IN, SLOW_FOR_IN };
  ForInType for_in_type() const { return for_in_type_; }
  void set_for_in_type(ForInType type) { for_in_type_ = type; }

  BailoutId BodyId() const { return body_id_; }
  BailoutId PrepareId() const { return prepare_id_; }
  virtual BailoutId ContinueId() const V8_OVERRIDE { return EntryId(); }
  virtual BailoutId StackCheckId() const V8_OVERRIDE { return body_id_; }

 protected:
  ForInStatement(Zone* zone, ZoneStringList* labels, int pos)
      : ForEachStatement(zone, labels, pos),
        for_in_type_(SLOW_FOR_IN),
        for_in_feedback_slot_(kInvalidFeedbackSlot),
        body_id_(GetNextId(zone)),
        prepare_id_(GetNextId(zone)) {
  }

  ForInType for_in_type_;
  int for_in_feedback_slot_;
  const BailoutId body_id_;
  const BailoutId prepare_id_;
};


class ForOfStatement V8_FINAL : public ForEachStatement {
 public:
  DECLARE_NODE_TYPE(ForOfStatement)

  void Initialize(Expression* each,
                  Expression* subject,
                  Statement* body,
                  Expression* assign_iterable,
                  Expression* assign_iterator,
                  Expression* next_result,
                  Expression* result_done,
                  Expression* assign_each) {
    ForEachStatement::Initialize(each, subject, body);
    assign_iterable_ = assign_iterable;
    assign_iterator_ = assign_iterator;
    next_result_ = next_result;
    result_done_ = result_done;
    assign_each_ = assign_each;
  }

  Expression* iterable() const {
    return subject();
  }

  // var iterable = subject;
  Expression* assign_iterable() const {
    return assign_iterable_;
  }

  // var iterator = iterable[Symbol.iterator]();
  Expression* assign_iterator() const {
    return assign_iterator_;
  }

  // var result = iterator.next();
  Expression* next_result() const {
    return next_result_;
  }

  // result.done
  Expression* result_done() const {
    return result_done_;
  }

  // each = result.value
  Expression* assign_each() const {
    return assign_each_;
  }

  virtual BailoutId ContinueId() const V8_OVERRIDE { return EntryId(); }
  virtual BailoutId StackCheckId() const V8_OVERRIDE { return BackEdgeId(); }

  BailoutId BackEdgeId() const { return back_edge_id_; }

 protected:
  ForOfStatement(Zone* zone, ZoneStringList* labels, int pos)
      : ForEachStatement(zone, labels, pos),
        assign_iterator_(NULL),
        next_result_(NULL),
        result_done_(NULL),
        assign_each_(NULL),
        back_edge_id_(GetNextId(zone)) {
  }

  Expression* assign_iterable_;
  Expression* assign_iterator_;
  Expression* next_result_;
  Expression* result_done_;
  Expression* assign_each_;
  const BailoutId back_edge_id_;
};


class ExpressionStatement V8_FINAL : public Statement {
 public:
  DECLARE_NODE_TYPE(ExpressionStatement)

  void set_expression(Expression* e) { expression_ = e; }
  Expression* expression() const { return expression_; }
  virtual bool IsJump() const V8_OVERRIDE { return expression_->IsThrow(); }

 protected:
  ExpressionStatement(Zone* zone, Expression* expression, int pos)
      : Statement(zone, pos), expression_(expression) { }

 private:
  Expression* expression_;
};


class JumpStatement : public Statement {
 public:
  virtual bool IsJump() const V8_FINAL V8_OVERRIDE { return true; }

 protected:
  explicit JumpStatement(Zone* zone, int pos) : Statement(zone, pos) {}
};


class ContinueStatement V8_FINAL : public JumpStatement {
 public:
  DECLARE_NODE_TYPE(ContinueStatement)

  IterationStatement* target() const { return target_; }

 protected:
  explicit ContinueStatement(Zone* zone, IterationStatement* target, int pos)
      : JumpStatement(zone, pos), target_(target) { }

 private:
  IterationStatement* target_;
};


class BreakStatement V8_FINAL : public JumpStatement {
 public:
  DECLARE_NODE_TYPE(BreakStatement)

  BreakableStatement* target() const { return target_; }

 protected:
  explicit BreakStatement(Zone* zone, BreakableStatement* target, int pos)
      : JumpStatement(zone, pos), target_(target) { }

 private:
  BreakableStatement* target_;
};


class ReturnStatement V8_FINAL : public JumpStatement {
 public:
  DECLARE_NODE_TYPE(ReturnStatement)

  Expression* expression() const { return expression_; }

 protected:
  explicit ReturnStatement(Zone* zone, Expression* expression, int pos)
      : JumpStatement(zone, pos), expression_(expression) { }

 private:
  Expression* expression_;
};


class WithStatement V8_FINAL : public Statement {
 public:
  DECLARE_NODE_TYPE(WithStatement)

  Scope* scope() { return scope_; }
  Expression* expression() const { return expression_; }
  Statement* statement() const { return statement_; }

 protected:
  WithStatement(
      Zone* zone, Scope* scope,
      Expression* expression, Statement* statement, int pos)
      : Statement(zone, pos),
        scope_(scope),
        expression_(expression),
        statement_(statement) { }

 private:
  Scope* scope_;
  Expression* expression_;
  Statement* statement_;
};


class CaseClause V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(CaseClause)

  bool is_default() const { return label_ == NULL; }
  Expression* label() const {
    CHECK(!is_default());
    return label_;
  }
  Label* body_target() { return &body_target_; }
  ZoneList<Statement*>* statements() const { return statements_; }

  BailoutId EntryId() const { return entry_id_; }

  // Type feedback information.
  TypeFeedbackId CompareId() { return compare_id_; }
  Type* compare_type() { return compare_type_; }
  void set_compare_type(Type* type) { compare_type_ = type; }

 private:
  CaseClause(Zone* zone,
             Expression* label,
             ZoneList<Statement*>* statements,
             int pos);

  Expression* label_;
  Label body_target_;
  ZoneList<Statement*>* statements_;
  Type* compare_type_;

  const TypeFeedbackId compare_id_;
  const BailoutId entry_id_;
};


class SwitchStatement V8_FINAL : public BreakableStatement {
 public:
  DECLARE_NODE_TYPE(SwitchStatement)

  void Initialize(Expression* tag, ZoneList<CaseClause*>* cases) {
    tag_ = tag;
    cases_ = cases;
  }

  Expression* tag() const { return tag_; }
  ZoneList<CaseClause*>* cases() const { return cases_; }

 protected:
  SwitchStatement(Zone* zone, ZoneStringList* labels, int pos)
      : BreakableStatement(zone, labels, TARGET_FOR_ANONYMOUS, pos),
        tag_(NULL),
        cases_(NULL) { }

 private:
  Expression* tag_;
  ZoneList<CaseClause*>* cases_;
};


// If-statements always have non-null references to their then- and
// else-parts. When parsing if-statements with no explicit else-part,
// the parser implicitly creates an empty statement. Use the
// HasThenStatement() and HasElseStatement() functions to check if a
// given if-statement has a then- or an else-part containing code.
class IfStatement V8_FINAL : public Statement {
 public:
  DECLARE_NODE_TYPE(IfStatement)

  bool HasThenStatement() const { return !then_statement()->IsEmpty(); }
  bool HasElseStatement() const { return !else_statement()->IsEmpty(); }

  Expression* condition() const { return condition_; }
  Statement* then_statement() const { return then_statement_; }
  Statement* else_statement() const { return else_statement_; }

  virtual bool IsJump() const V8_OVERRIDE {
    return HasThenStatement() && then_statement()->IsJump()
        && HasElseStatement() && else_statement()->IsJump();
  }

  BailoutId IfId() const { return if_id_; }
  BailoutId ThenId() const { return then_id_; }
  BailoutId ElseId() const { return else_id_; }

 protected:
  IfStatement(Zone* zone,
              Expression* condition,
              Statement* then_statement,
              Statement* else_statement,
              int pos)
      : Statement(zone, pos),
        condition_(condition),
        then_statement_(then_statement),
        else_statement_(else_statement),
        if_id_(GetNextId(zone)),
        then_id_(GetNextId(zone)),
        else_id_(GetNextId(zone)) {
  }

 private:
  Expression* condition_;
  Statement* then_statement_;
  Statement* else_statement_;
  const BailoutId if_id_;
  const BailoutId then_id_;
  const BailoutId else_id_;
};


// NOTE: TargetCollectors are represented as nodes to fit in the target
// stack in the compiler; this should probably be reworked.
class TargetCollector V8_FINAL : public AstNode {
 public:
  explicit TargetCollector(Zone* zone)
      : AstNode(RelocInfo::kNoPosition), targets_(0, zone) { }

  // Adds a jump target to the collector. The collector stores a pointer not
  // a copy of the target to make binding work, so make sure not to pass in
  // references to something on the stack.
  void AddTarget(Label* target, Zone* zone);

  // Virtual behaviour. TargetCollectors are never part of the AST.
  virtual void Accept(AstVisitor* v) V8_OVERRIDE { UNREACHABLE(); }
  virtual NodeType node_type() const V8_OVERRIDE { return kInvalid; }
  virtual TargetCollector* AsTargetCollector() V8_OVERRIDE { return this; }

  ZoneList<Label*>* targets() { return &targets_; }

 private:
  ZoneList<Label*> targets_;
};


class TryStatement : public Statement {
 public:
  void set_escaping_targets(ZoneList<Label*>* targets) {
    escaping_targets_ = targets;
  }

  int index() const { return index_; }
  Block* try_block() const { return try_block_; }
  ZoneList<Label*>* escaping_targets() const { return escaping_targets_; }

 protected:
  TryStatement(Zone* zone, int index, Block* try_block, int pos)
      : Statement(zone, pos),
        index_(index),
        try_block_(try_block),
        escaping_targets_(NULL) { }

 private:
  // Unique (per-function) index of this handler.  This is not an AST ID.
  int index_;

  Block* try_block_;
  ZoneList<Label*>* escaping_targets_;
};


class TryCatchStatement V8_FINAL : public TryStatement {
 public:
  DECLARE_NODE_TYPE(TryCatchStatement)

  Scope* scope() { return scope_; }
  Variable* variable() { return variable_; }
  Block* catch_block() const { return catch_block_; }

 protected:
  TryCatchStatement(Zone* zone,
                    int index,
                    Block* try_block,
                    Scope* scope,
                    Variable* variable,
                    Block* catch_block,
                    int pos)
      : TryStatement(zone, index, try_block, pos),
        scope_(scope),
        variable_(variable),
        catch_block_(catch_block) {
  }

 private:
  Scope* scope_;
  Variable* variable_;
  Block* catch_block_;
};


class TryFinallyStatement V8_FINAL : public TryStatement {
 public:
  DECLARE_NODE_TYPE(TryFinallyStatement)

  Block* finally_block() const { return finally_block_; }

 protected:
  TryFinallyStatement(
      Zone* zone, int index, Block* try_block, Block* finally_block, int pos)
      : TryStatement(zone, index, try_block, pos),
        finally_block_(finally_block) { }

 private:
  Block* finally_block_;
};


class DebuggerStatement V8_FINAL : public Statement {
 public:
  DECLARE_NODE_TYPE(DebuggerStatement)

 protected:
  explicit DebuggerStatement(Zone* zone, int pos): Statement(zone, pos) {}
};


class EmptyStatement V8_FINAL : public Statement {
 public:
  DECLARE_NODE_TYPE(EmptyStatement)

 protected:
  explicit EmptyStatement(Zone* zone, int pos): Statement(zone, pos) {}
};


class Literal V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(Literal)

  virtual bool IsPropertyName() const V8_OVERRIDE {
    if (value_->IsInternalizedString()) {
      uint32_t ignored;
      return !String::cast(*value_)->AsArrayIndex(&ignored);
    }
    return false;
  }

  Handle<String> AsPropertyName() {
    ASSERT(IsPropertyName());
    return Handle<String>::cast(value_);
  }

  virtual bool ToBooleanIsTrue() const V8_OVERRIDE {
    return value_->BooleanValue();
  }
  virtual bool ToBooleanIsFalse() const V8_OVERRIDE {
    return !value_->BooleanValue();
  }

  Handle<Object> value() const { return value_; }

  // Support for using Literal as a HashMap key. NOTE: Currently, this works
  // only for string and number literals!
  uint32_t Hash() { return ToString()->Hash(); }

  static bool Match(void* literal1, void* literal2) {
    Handle<String> s1 = static_cast<Literal*>(literal1)->ToString();
    Handle<String> s2 = static_cast<Literal*>(literal2)->ToString();
    return String::Equals(s1, s2);
  }

  TypeFeedbackId LiteralFeedbackId() const { return reuse(id()); }

 protected:
  Literal(Zone* zone, Handle<Object> value, int position)
      : Expression(zone, position),
        value_(value),
        isolate_(zone->isolate()) { }

 private:
  Handle<String> ToString();

  Handle<Object> value_;
  // TODO(dcarney): remove.  this is only needed for Match and Hash.
  Isolate* isolate_;
};


// Base class for literals that needs space in the corresponding JSFunction.
class MaterializedLiteral : public Expression {
 public:
  virtual MaterializedLiteral* AsMaterializedLiteral() { return this; }

  int literal_index() { return literal_index_; }

  int depth() const {
    // only callable after initialization.
    ASSERT(depth_ >= 1);
    return depth_;
  }

 protected:
  MaterializedLiteral(Zone* zone,
                      int literal_index,
                      int pos)
      : Expression(zone, pos),
        literal_index_(literal_index),
        is_simple_(false),
        depth_(0) {}

  // A materialized literal is simple if the values consist of only
  // constants and simple object and array literals.
  bool is_simple() const { return is_simple_; }
  void set_is_simple(bool is_simple) { is_simple_ = is_simple; }
  friend class CompileTimeValue;

  void set_depth(int depth) {
    ASSERT(depth >= 1);
    depth_ = depth;
  }

  // Populate the constant properties/elements fixed array.
  void BuildConstants(Isolate* isolate);
  friend class ArrayLiteral;
  friend class ObjectLiteral;

  // If the expression is a literal, return the literal value;
  // if the expression is a materialized literal and is simple return a
  // compile time value as encoded by CompileTimeValue::GetValue().
  // Otherwise, return undefined literal as the placeholder
  // in the object literal boilerplate.
  Handle<Object> GetBoilerplateValue(Expression* expression, Isolate* isolate);

 private:
  int literal_index_;
  bool is_simple_;
  int depth_;
};


// Property is used for passing information
// about an object literal's properties from the parser
// to the code generator.
class ObjectLiteralProperty V8_FINAL : public ZoneObject {
 public:
  enum Kind {
    CONSTANT,              // Property with constant value (compile time).
    COMPUTED,              // Property with computed value (execution time).
    MATERIALIZED_LITERAL,  // Property value is a materialized literal.
    GETTER, SETTER,        // Property is an accessor function.
    PROTOTYPE              // Property is __proto__.
  };

  ObjectLiteralProperty(Zone* zone, Literal* key, Expression* value);

  Literal* key() { return key_; }
  Expression* value() { return value_; }
  Kind kind() { return kind_; }

  // Type feedback information.
  void RecordTypeFeedback(TypeFeedbackOracle* oracle);
  bool IsMonomorphic() { return !receiver_type_.is_null(); }
  Handle<Map> GetReceiverType() { return receiver_type_; }

  bool IsCompileTimeValue();

  void set_emit_store(bool emit_store);
  bool emit_store();

 protected:
  template<class> friend class AstNodeFactory;

  ObjectLiteralProperty(Zone* zone, bool is_getter, FunctionLiteral* value);
  void set_key(Literal* key) { key_ = key; }

 private:
  Literal* key_;
  Expression* value_;
  Kind kind_;
  bool emit_store_;
  Handle<Map> receiver_type_;
};


// An object literal has a boilerplate object that is used
// for minimizing the work when constructing it at runtime.
class ObjectLiteral V8_FINAL : public MaterializedLiteral {
 public:
  typedef ObjectLiteralProperty Property;

  DECLARE_NODE_TYPE(ObjectLiteral)

  Handle<FixedArray> constant_properties() const {
    return constant_properties_;
  }
  ZoneList<Property*>* properties() const { return properties_; }
  bool fast_elements() const { return fast_elements_; }
  bool may_store_doubles() const { return may_store_doubles_; }
  bool has_function() const { return has_function_; }

  // Decide if a property should be in the object boilerplate.
  static bool IsBoilerplateProperty(Property* property);

  // Populate the constant properties fixed array.
  void BuildConstantProperties(Isolate* isolate);

  // Mark all computed expressions that are bound to a key that
  // is shadowed by a later occurrence of the same key. For the
  // marked expressions, no store code is emitted.
  void CalculateEmitStore(Zone* zone);

  enum Flags {
    kNoFlags = 0,
    kFastElements = 1,
    kHasFunction = 1 << 1
  };

  struct Accessors: public ZoneObject {
    Accessors() : getter(NULL), setter(NULL) { }
    Expression* getter;
    Expression* setter;
  };

 protected:
  ObjectLiteral(Zone* zone,
                ZoneList<Property*>* properties,
                int literal_index,
                int boilerplate_properties,
                bool has_function,
                int pos)
      : MaterializedLiteral(zone, literal_index, pos),
        properties_(properties),
        boilerplate_properties_(boilerplate_properties),
        fast_elements_(false),
        may_store_doubles_(false),
        has_function_(has_function) {}

 private:
  Handle<FixedArray> constant_properties_;
  ZoneList<Property*>* properties_;
  int boilerplate_properties_;
  bool fast_elements_;
  bool may_store_doubles_;
  bool has_function_;
};


// Node for capturing a regexp literal.
class RegExpLiteral V8_FINAL : public MaterializedLiteral {
 public:
  DECLARE_NODE_TYPE(RegExpLiteral)

  Handle<String> pattern() const { return pattern_; }
  Handle<String> flags() const { return flags_; }

 protected:
  RegExpLiteral(Zone* zone,
                Handle<String> pattern,
                Handle<String> flags,
                int literal_index,
                int pos)
      : MaterializedLiteral(zone, literal_index, pos),
        pattern_(pattern),
        flags_(flags) {
    set_depth(1);
  }

 private:
  Handle<String> pattern_;
  Handle<String> flags_;
};


// An array literal has a literals object that is used
// for minimizing the work when constructing it at runtime.
class ArrayLiteral V8_FINAL : public MaterializedLiteral {
 public:
  DECLARE_NODE_TYPE(ArrayLiteral)

  Handle<FixedArray> constant_elements() const { return constant_elements_; }
  ZoneList<Expression*>* values() const { return values_; }

  // Return an AST id for an element that is used in simulate instructions.
  BailoutId GetIdForElement(int i) {
    return BailoutId(first_element_id_.ToInt() + i);
  }

  // Populate the constant elements fixed array.
  void BuildConstantElements(Isolate* isolate);

  enum Flags {
    kNoFlags = 0,
    kShallowElements = 1,
    kDisableMementos = 1 << 1
  };

 protected:
  ArrayLiteral(Zone* zone,
               ZoneList<Expression*>* values,
               int literal_index,
               int pos)
      : MaterializedLiteral(zone, literal_index, pos),
        values_(values),
        first_element_id_(ReserveIdRange(zone, values->length())) {}

 private:
  Handle<FixedArray> constant_elements_;
  ZoneList<Expression*>* values_;
  const BailoutId first_element_id_;
};


class VariableProxy V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(VariableProxy)

  virtual bool IsValidReferenceExpression() const V8_OVERRIDE {
    return var_ == NULL ? true : var_->IsValidReference();
  }

  bool IsVariable(Handle<String> n) const {
    return !is_this() && name().is_identical_to(n);
  }

  bool IsArguments() const { return var_ != NULL && var_->is_arguments(); }

  bool IsLValue() const { return is_lvalue_; }

  Handle<String> name() const { return name_; }
  Variable* var() const { return var_; }
  bool is_this() const { return is_this_; }
  Interface* interface() const { return interface_; }


  void MarkAsTrivial() { is_trivial_ = true; }
  void MarkAsLValue() { is_lvalue_ = true; }

  // Bind this proxy to the variable var. Interfaces must match.
  void BindTo(Variable* var);

 protected:
  VariableProxy(Zone* zone, Variable* var, int position);

  VariableProxy(Zone* zone,
                Handle<String> name,
                bool is_this,
                Interface* interface,
                int position);

  Handle<String> name_;
  Variable* var_;  // resolved variable, or NULL
  bool is_this_;
  bool is_trivial_;
  // True if this variable proxy is being used in an assignment
  // or with a increment/decrement operator.
  bool is_lvalue_;
  Interface* interface_;
};


class Property V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(Property)

  virtual bool IsValidReferenceExpression() const V8_OVERRIDE { return true; }

  Expression* obj() const { return obj_; }
  Expression* key() const { return key_; }

  BailoutId LoadId() const { return load_id_; }

  bool IsStringAccess() const { return is_string_access_; }
  bool IsFunctionPrototype() const { return is_function_prototype_; }

  // Type feedback information.
  virtual bool IsMonomorphic() V8_OVERRIDE {
    return receiver_types_.length() == 1;
  }
  virtual SmallMapList* GetReceiverTypes() V8_OVERRIDE {
    return &receiver_types_;
  }
  virtual KeyedAccessStoreMode GetStoreMode() V8_OVERRIDE {
    return STANDARD_STORE;
  }
  bool IsUninitialized() { return !is_for_call_ && is_uninitialized_; }
  bool HasNoTypeInformation() {
    return is_uninitialized_;
  }
  void set_is_uninitialized(bool b) { is_uninitialized_ = b; }
  void set_is_string_access(bool b) { is_string_access_ = b; }
  void set_is_function_prototype(bool b) { is_function_prototype_ = b; }
  void mark_for_call() { is_for_call_ = true; }
  bool IsForCall() { return is_for_call_; }

  TypeFeedbackId PropertyFeedbackId() { return reuse(id()); }

 protected:
  Property(Zone* zone,
           Expression* obj,
           Expression* key,
           int pos)
      : Expression(zone, pos),
        obj_(obj),
        key_(key),
        load_id_(GetNextId(zone)),
        is_for_call_(false),
        is_uninitialized_(false),
        is_string_access_(false),
        is_function_prototype_(false) { }

 private:
  Expression* obj_;
  Expression* key_;
  const BailoutId load_id_;

  SmallMapList receiver_types_;
  bool is_for_call_ : 1;
  bool is_uninitialized_ : 1;
  bool is_string_access_ : 1;
  bool is_function_prototype_ : 1;
};


class Call V8_FINAL : public Expression, public FeedbackSlotInterface {
 public:
  DECLARE_NODE_TYPE(Call)

  Expression* expression() const { return expression_; }
  ZoneList<Expression*>* arguments() const { return arguments_; }

  // Type feedback information.
  virtual int ComputeFeedbackSlotCount() { return 1; }
  virtual void SetFirstFeedbackSlot(int slot) {
    call_feedback_slot_ = slot;
  }

  bool HasCallFeedbackSlot() const {
    return call_feedback_slot_ != kInvalidFeedbackSlot;
  }
  int CallFeedbackSlot() const { return call_feedback_slot_; }

  virtual SmallMapList* GetReceiverTypes() V8_OVERRIDE {
    if (expression()->IsProperty()) {
      return expression()->AsProperty()->GetReceiverTypes();
    }
    return NULL;
  }

  virtual bool IsMonomorphic() V8_OVERRIDE {
    if (expression()->IsProperty()) {
      return expression()->AsProperty()->IsMonomorphic();
    }
    return !target_.is_null();
  }

  bool global_call() const {
    VariableProxy* proxy = expression_->AsVariableProxy();
    return proxy != NULL && proxy->var()->IsUnallocated();
  }

  bool known_global_function() const {
    return global_call() && !target_.is_null();
  }

  Handle<JSFunction> target() { return target_; }

  Handle<Cell> cell() { return cell_; }

  Handle<AllocationSite> allocation_site() { return allocation_site_; }

  void set_target(Handle<JSFunction> target) { target_ = target; }
  void set_allocation_site(Handle<AllocationSite> site) {
    allocation_site_ = site;
  }
  bool ComputeGlobalTarget(Handle<GlobalObject> global, LookupResult* lookup);

  BailoutId ReturnId() const { return return_id_; }

  enum CallType {
    POSSIBLY_EVAL_CALL,
    GLOBAL_CALL,
    LOOKUP_SLOT_CALL,
    PROPERTY_CALL,
    OTHER_CALL
  };

  // Helpers to determine how to handle the call.
  CallType GetCallType(Isolate* isolate) const;
  bool IsUsingCallFeedbackSlot(Isolate* isolate) const;

#ifdef DEBUG
  // Used to assert that the FullCodeGenerator records the return site.
  bool return_is_recorded_;
#endif

 protected:
  Call(Zone* zone,
       Expression* expression,
       ZoneList<Expression*>* arguments,
       int pos)
      : Expression(zone, pos),
        expression_(expression),
        arguments_(arguments),
        call_feedback_slot_(kInvalidFeedbackSlot),
        return_id_(GetNextId(zone)) {
    if (expression->IsProperty()) {
      expression->AsProperty()->mark_for_call();
    }
  }

 private:
  Expression* expression_;
  ZoneList<Expression*>* arguments_;

  Handle<JSFunction> target_;
  Handle<Cell> cell_;
  Handle<AllocationSite> allocation_site_;
  int call_feedback_slot_;

  const BailoutId return_id_;
};


class CallNew V8_FINAL : public Expression, public FeedbackSlotInterface {
 public:
  DECLARE_NODE_TYPE(CallNew)

  Expression* expression() const { return expression_; }
  ZoneList<Expression*>* arguments() const { return arguments_; }

  // Type feedback information.
  virtual int ComputeFeedbackSlotCount() {
    return FLAG_pretenuring_call_new ? 2 : 1;
  }
  virtual void SetFirstFeedbackSlot(int slot) {
    callnew_feedback_slot_ = slot;
  }

  int CallNewFeedbackSlot() {
    ASSERT(callnew_feedback_slot_ != kInvalidFeedbackSlot);
    return callnew_feedback_slot_;
  }
  int AllocationSiteFeedbackSlot() {
    ASSERT(callnew_feedback_slot_ != kInvalidFeedbackSlot);
    ASSERT(FLAG_pretenuring_call_new);
    return callnew_feedback_slot_ + 1;
  }

  void RecordTypeFeedback(TypeFeedbackOracle* oracle);
  virtual bool IsMonomorphic() V8_OVERRIDE { return is_monomorphic_; }
  Handle<JSFunction> target() const { return target_; }
  ElementsKind elements_kind() const { return elements_kind_; }
  Handle<AllocationSite> allocation_site() const {
    return allocation_site_;
  }

  static int feedback_slots() { return 1; }

  BailoutId ReturnId() const { return return_id_; }

 protected:
  CallNew(Zone* zone,
          Expression* expression,
          ZoneList<Expression*>* arguments,
          int pos)
      : Expression(zone, pos),
        expression_(expression),
        arguments_(arguments),
        is_monomorphic_(false),
        elements_kind_(GetInitialFastElementsKind()),
        callnew_feedback_slot_(kInvalidFeedbackSlot),
        return_id_(GetNextId(zone)) { }

 private:
  Expression* expression_;
  ZoneList<Expression*>* arguments_;

  bool is_monomorphic_;
  Handle<JSFunction> target_;
  ElementsKind elements_kind_;
  Handle<AllocationSite> allocation_site_;
  int callnew_feedback_slot_;

  const BailoutId return_id_;
};


// The CallRuntime class does not represent any official JavaScript
// language construct. Instead it is used to call a C or JS function
// with a set of arguments. This is used from the builtins that are
// implemented in JavaScript (see "v8natives.js").
class CallRuntime V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(CallRuntime)

  Handle<String> name() const { return name_; }
  const Runtime::Function* function() const { return function_; }
  ZoneList<Expression*>* arguments() const { return arguments_; }
  bool is_jsruntime() const { return function_ == NULL; }

  TypeFeedbackId CallRuntimeFeedbackId() const { return reuse(id()); }

 protected:
  CallRuntime(Zone* zone,
              Handle<String> name,
              const Runtime::Function* function,
              ZoneList<Expression*>* arguments,
              int pos)
      : Expression(zone, pos),
        name_(name),
        function_(function),
        arguments_(arguments) { }

 private:
  Handle<String> name_;
  const Runtime::Function* function_;
  ZoneList<Expression*>* arguments_;
};


class UnaryOperation V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(UnaryOperation)

  Token::Value op() const { return op_; }
  Expression* expression() const { return expression_; }

  BailoutId MaterializeTrueId() { return materialize_true_id_; }
  BailoutId MaterializeFalseId() { return materialize_false_id_; }

  virtual void RecordToBooleanTypeFeedback(
      TypeFeedbackOracle* oracle) V8_OVERRIDE;

 protected:
  UnaryOperation(Zone* zone,
                 Token::Value op,
                 Expression* expression,
                 int pos)
      : Expression(zone, pos),
        op_(op),
        expression_(expression),
        materialize_true_id_(GetNextId(zone)),
        materialize_false_id_(GetNextId(zone)) {
    ASSERT(Token::IsUnaryOp(op));
  }

 private:
  Token::Value op_;
  Expression* expression_;

  // For unary not (Token::NOT), the AST ids where true and false will
  // actually be materialized, respectively.
  const BailoutId materialize_true_id_;
  const BailoutId materialize_false_id_;
};


class BinaryOperation V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(BinaryOperation)

  virtual bool ResultOverwriteAllowed() const V8_OVERRIDE;

  Token::Value op() const { return op_; }
  Expression* left() const { return left_; }
  Expression* right() const { return right_; }
  Handle<AllocationSite> allocation_site() const { return allocation_site_; }
  void set_allocation_site(Handle<AllocationSite> allocation_site) {
    allocation_site_ = allocation_site;
  }

  BailoutId RightId() const { return right_id_; }

  TypeFeedbackId BinaryOperationFeedbackId() const { return reuse(id()); }
  Maybe<int> fixed_right_arg() const { return fixed_right_arg_; }
  void set_fixed_right_arg(Maybe<int> arg) { fixed_right_arg_ = arg; }

  virtual void RecordToBooleanTypeFeedback(
      TypeFeedbackOracle* oracle) V8_OVERRIDE;

 protected:
  BinaryOperation(Zone* zone,
                  Token::Value op,
                  Expression* left,
                  Expression* right,
                  int pos)
      : Expression(zone, pos),
        op_(op),
        left_(left),
        right_(right),
        right_id_(GetNextId(zone)) {
    ASSERT(Token::IsBinaryOp(op));
  }

 private:
  Token::Value op_;
  Expression* left_;
  Expression* right_;
  Handle<AllocationSite> allocation_site_;

  // TODO(rossberg): the fixed arg should probably be represented as a Constant
  // type for the RHS.
  Maybe<int> fixed_right_arg_;

  // The short-circuit logical operations need an AST ID for their
  // right-hand subexpression.
  const BailoutId right_id_;
};


class CountOperation V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(CountOperation)

  bool is_prefix() const { return is_prefix_; }
  bool is_postfix() const { return !is_prefix_; }

  Token::Value op() const { return op_; }
  Token::Value binary_op() {
    return (op() == Token::INC) ? Token::ADD : Token::SUB;
  }

  Expression* expression() const { return expression_; }

  virtual bool IsMonomorphic() V8_OVERRIDE {
    return receiver_types_.length() == 1;
  }
  virtual SmallMapList* GetReceiverTypes() V8_OVERRIDE {
    return &receiver_types_;
  }
  virtual KeyedAccessStoreMode GetStoreMode() V8_OVERRIDE {
    return store_mode_;
  }
  Type* type() const { return type_; }
  void set_store_mode(KeyedAccessStoreMode mode) { store_mode_ = mode; }
  void set_type(Type* type) { type_ = type; }

  BailoutId AssignmentId() const { return assignment_id_; }

  TypeFeedbackId CountBinOpFeedbackId() const { return count_id_; }
  TypeFeedbackId CountStoreFeedbackId() const { return reuse(id()); }

 protected:
  CountOperation(Zone* zone,
                 Token::Value op,
                 bool is_prefix,
                 Expression* expr,
                 int pos)
      : Expression(zone, pos),
        op_(op),
        is_prefix_(is_prefix),
        store_mode_(STANDARD_STORE),
        expression_(expr),
        assignment_id_(GetNextId(zone)),
        count_id_(GetNextId(zone)) {}

 private:
  Token::Value op_;
  bool is_prefix_ : 1;
  KeyedAccessStoreMode store_mode_ : 5;  // Windows treats as signed,
                                         // must have extra bit.
  Type* type_;

  Expression* expression_;
  const BailoutId assignment_id_;
  const TypeFeedbackId count_id_;
  SmallMapList receiver_types_;
};


class CompareOperation V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(CompareOperation)

  Token::Value op() const { return op_; }
  Expression* left() const { return left_; }
  Expression* right() const { return right_; }

  // Type feedback information.
  TypeFeedbackId CompareOperationFeedbackId() const { return reuse(id()); }
  Type* combined_type() const { return combined_type_; }
  void set_combined_type(Type* type) { combined_type_ = type; }

  // Match special cases.
  bool IsLiteralCompareTypeof(Expression** expr, Handle<String>* check);
  bool IsLiteralCompareUndefined(Expression** expr, Isolate* isolate);
  bool IsLiteralCompareNull(Expression** expr);

 protected:
  CompareOperation(Zone* zone,
                   Token::Value op,
                   Expression* left,
                   Expression* right,
                   int pos)
      : Expression(zone, pos),
        op_(op),
        left_(left),
        right_(right),
        combined_type_(Type::None(zone)) {
    ASSERT(Token::IsCompareOp(op));
  }

 private:
  Token::Value op_;
  Expression* left_;
  Expression* right_;

  Type* combined_type_;
};


class Conditional V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(Conditional)

  Expression* condition() const { return condition_; }
  Expression* then_expression() const { return then_expression_; }
  Expression* else_expression() const { return else_expression_; }

  BailoutId ThenId() const { return then_id_; }
  BailoutId ElseId() const { return else_id_; }

 protected:
  Conditional(Zone* zone,
              Expression* condition,
              Expression* then_expression,
              Expression* else_expression,
              int position)
      : Expression(zone, position),
        condition_(condition),
        then_expression_(then_expression),
        else_expression_(else_expression),
        then_id_(GetNextId(zone)),
        else_id_(GetNextId(zone)) { }

 private:
  Expression* condition_;
  Expression* then_expression_;
  Expression* else_expression_;
  const BailoutId then_id_;
  const BailoutId else_id_;
};


class Assignment V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(Assignment)

  Assignment* AsSimpleAssignment() { return !is_compound() ? this : NULL; }

  Token::Value binary_op() const;

  Token::Value op() const { return op_; }
  Expression* target() const { return target_; }
  Expression* value() const { return value_; }
  BinaryOperation* binary_operation() const { return binary_operation_; }

  // This check relies on the definition order of token in token.h.
  bool is_compound() const { return op() > Token::ASSIGN; }

  BailoutId AssignmentId() const { return assignment_id_; }

  // Type feedback information.
  TypeFeedbackId AssignmentFeedbackId() { return reuse(id()); }
  virtual bool IsMonomorphic() V8_OVERRIDE {
    return receiver_types_.length() == 1;
  }
  bool IsUninitialized() { return is_uninitialized_; }
  bool HasNoTypeInformation() {
    return is_uninitialized_;
  }
  virtual SmallMapList* GetReceiverTypes() V8_OVERRIDE {
    return &receiver_types_;
  }
  virtual KeyedAccessStoreMode GetStoreMode() V8_OVERRIDE {
    return store_mode_;
  }
  void set_is_uninitialized(bool b) { is_uninitialized_ = b; }
  void set_store_mode(KeyedAccessStoreMode mode) { store_mode_ = mode; }

 protected:
  Assignment(Zone* zone,
             Token::Value op,
             Expression* target,
             Expression* value,
             int pos);

  template<class Visitor>
  void Init(Zone* zone, AstNodeFactory<Visitor>* factory) {
    ASSERT(Token::IsAssignmentOp(op_));
    if (is_compound()) {
      binary_operation_ = factory->NewBinaryOperation(
          binary_op(), target_, value_, position() + 1);
    }
  }

 private:
  Token::Value op_;
  Expression* target_;
  Expression* value_;
  BinaryOperation* binary_operation_;
  const BailoutId assignment_id_;

  bool is_uninitialized_ : 1;
  KeyedAccessStoreMode store_mode_ : 5;  // Windows treats as signed,
                                         // must have extra bit.
  SmallMapList receiver_types_;
};


class Yield V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(Yield)

  enum Kind {
    INITIAL,     // The initial yield that returns the unboxed generator object.
    SUSPEND,     // A normal yield: { value: EXPRESSION, done: false }
    DELEGATING,  // A yield*.
    FINAL        // A return: { value: EXPRESSION, done: true }
  };

  Expression* generator_object() const { return generator_object_; }
  Expression* expression() const { return expression_; }
  Kind yield_kind() const { return yield_kind_; }

  // Delegating yield surrounds the "yield" in a "try/catch".  This index
  // locates the catch handler in the handler table, and is equivalent to
  // TryCatchStatement::index().
  int index() const {
    ASSERT(yield_kind() == DELEGATING);
    return index_;
  }
  void set_index(int index) {
    ASSERT(yield_kind() == DELEGATING);
    index_ = index;
  }

 protected:
  Yield(Zone* zone,
        Expression* generator_object,
        Expression* expression,
        Kind yield_kind,
        int pos)
      : Expression(zone, pos),
        generator_object_(generator_object),
        expression_(expression),
        yield_kind_(yield_kind),
        index_(-1) { }

 private:
  Expression* generator_object_;
  Expression* expression_;
  Kind yield_kind_;
  int index_;
};


class Throw V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(Throw)

  Expression* exception() const { return exception_; }

 protected:
  Throw(Zone* zone, Expression* exception, int pos)
      : Expression(zone, pos), exception_(exception) {}

 private:
  Expression* exception_;
};


class FunctionLiteral V8_FINAL : public Expression {
 public:
  enum FunctionType {
    ANONYMOUS_EXPRESSION,
    NAMED_EXPRESSION,
    DECLARATION
  };

  enum ParameterFlag {
    kNoDuplicateParameters = 0,
    kHasDuplicateParameters = 1
  };

  enum IsFunctionFlag {
    kGlobalOrEval,
    kIsFunction
  };

  enum IsParenthesizedFlag {
    kIsParenthesized,
    kNotParenthesized
  };

  enum IsGeneratorFlag {
    kIsGenerator,
    kNotGenerator
  };

  DECLARE_NODE_TYPE(FunctionLiteral)

  Handle<String> name() const { return name_; }
  Scope* scope() const { return scope_; }
  ZoneList<Statement*>* body() const { return body_; }
  void set_function_token_position(int pos) { function_token_position_ = pos; }
  int function_token_position() const { return function_token_position_; }
  int start_position() const;
  int end_position() const;
  int SourceSize() const { return end_position() - start_position(); }
  bool is_expression() const { return IsExpression::decode(bitfield_); }
  bool is_anonymous() const { return IsAnonymous::decode(bitfield_); }
  StrictMode strict_mode() const;

  int materialized_literal_count() { return materialized_literal_count_; }
  int expected_property_count() { return expected_property_count_; }
  int handler_count() { return handler_count_; }
  int parameter_count() { return parameter_count_; }

  bool AllowsLazyCompilation();
  bool AllowsLazyCompilationWithoutContext();

  void InitializeSharedInfo(Handle<Code> code);

  Handle<String> debug_name() const {
    if (name_->length() > 0) return name_;
    return inferred_name();
  }

  Handle<String> inferred_name() const { return inferred_name_; }
  void set_inferred_name(Handle<String> inferred_name) {
    inferred_name_ = inferred_name;
  }

  // shared_info may be null if it's not cached in full code.
  Handle<SharedFunctionInfo> shared_info() { return shared_info_; }

  bool pretenure() { return Pretenure::decode(bitfield_); }
  void set_pretenure() { bitfield_ |= Pretenure::encode(true); }

  bool has_duplicate_parameters() {
    return HasDuplicateParameters::decode(bitfield_);
  }

  bool is_function() { return IsFunction::decode(bitfield_) == kIsFunction; }

  // This is used as a heuristic on when to eagerly compile a function
  // literal. We consider the following constructs as hints that the
  // function will be called immediately:
  // - (function() { ... })();
  // - var x = function() { ... }();
  bool is_parenthesized() {
    return IsParenthesized::decode(bitfield_) == kIsParenthesized;
  }
  void set_parenthesized() {
    bitfield_ = IsParenthesized::update(bitfield_, kIsParenthesized);
  }

  bool is_generator() {
    return IsGenerator::decode(bitfield_) == kIsGenerator;
  }

  int ast_node_count() { return ast_properties_.node_count(); }
  AstProperties::Flags* flags() { return ast_properties_.flags(); }
  void set_ast_properties(AstProperties* ast_properties) {
    ast_properties_ = *ast_properties;
  }
  int slot_count() {
    return ast_properties_.feedback_slots();
  }
  bool dont_optimize() { return dont_optimize_reason_ != kNoReason; }
  BailoutReason dont_optimize_reason() { return dont_optimize_reason_; }
  void set_dont_optimize_reason(BailoutReason reason) {
    dont_optimize_reason_ = reason;
  }

 protected:
  FunctionLiteral(Zone* zone,
                  Handle<String> name,
                  Scope* scope,
                  ZoneList<Statement*>* body,
                  int materialized_literal_count,
                  int expected_property_count,
                  int handler_count,
                  int parameter_count,
                  FunctionType function_type,
                  ParameterFlag has_duplicate_parameters,
                  IsFunctionFlag is_function,
                  IsParenthesizedFlag is_parenthesized,
                  IsGeneratorFlag is_generator,
                  int position)
      : Expression(zone, position),
        name_(name),
        scope_(scope),
        body_(body),
        inferred_name_(zone->isolate()->factory()->empty_string()),
        dont_optimize_reason_(kNoReason),
        materialized_literal_count_(materialized_literal_count),
        expected_property_count_(expected_property_count),
        handler_count_(handler_count),
        parameter_count_(parameter_count),
        function_token_position_(RelocInfo::kNoPosition) {
    bitfield_ =
        IsExpression::encode(function_type != DECLARATION) |
        IsAnonymous::encode(function_type == ANONYMOUS_EXPRESSION) |
        Pretenure::encode(false) |
        HasDuplicateParameters::encode(has_duplicate_parameters) |
        IsFunction::encode(is_function) |
        IsParenthesized::encode(is_parenthesized) |
        IsGenerator::encode(is_generator);
  }

 private:
  Handle<String> name_;
  Handle<SharedFunctionInfo> shared_info_;
  Scope* scope_;
  ZoneList<Statement*>* body_;
  Handle<String> inferred_name_;
  AstProperties ast_properties_;
  BailoutReason dont_optimize_reason_;

  int materialized_literal_count_;
  int expected_property_count_;
  int handler_count_;
  int parameter_count_;
  int function_token_position_;

  unsigned bitfield_;
  class IsExpression: public BitField<bool, 0, 1> {};
  class IsAnonymous: public BitField<bool, 1, 1> {};
  class Pretenure: public BitField<bool, 2, 1> {};
  class HasDuplicateParameters: public BitField<ParameterFlag, 3, 1> {};
  class IsFunction: public BitField<IsFunctionFlag, 4, 1> {};
  class IsParenthesized: public BitField<IsParenthesizedFlag, 5, 1> {};
  class IsGenerator: public BitField<IsGeneratorFlag, 6, 1> {};
};


class NativeFunctionLiteral V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(NativeFunctionLiteral)

  Handle<String> name() const { return name_; }
  v8::Extension* extension() const { return extension_; }

 protected:
  NativeFunctionLiteral(
      Zone* zone, Handle<String> name, v8::Extension* extension, int pos)
      : Expression(zone, pos), name_(name), extension_(extension) {}

 private:
  Handle<String> name_;
  v8::Extension* extension_;
};


class ThisFunction V8_FINAL : public Expression {
 public:
  DECLARE_NODE_TYPE(ThisFunction)

 protected:
  explicit ThisFunction(Zone* zone, int pos): Expression(zone, pos) {}
};

#undef DECLARE_NODE_TYPE


// ----------------------------------------------------------------------------
// Regular expressions


class RegExpVisitor BASE_EMBEDDED {
 public:
  virtual ~RegExpVisitor() { }
#define MAKE_CASE(Name)                                              \
  virtual void* Visit##Name(RegExp##Name*, void* data) = 0;
  FOR_EACH_REG_EXP_TREE_TYPE(MAKE_CASE)
#undef MAKE_CASE
};


class RegExpTree : public ZoneObject {
 public:
  static const int kInfinity = kMaxInt;
  virtual ~RegExpTree() {}
  virtual void* Accept(RegExpVisitor* visitor, void* data) = 0;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) = 0;
  virtual bool IsTextElement() { return false; }
  virtual bool IsAnchoredAtStart() { return false; }
  virtual bool IsAnchoredAtEnd() { return false; }
  virtual int min_match() = 0;
  virtual int max_match() = 0;
  // Returns the interval of registers used for captures within this
  // expression.
  virtual Interval CaptureRegisters() { return Interval::Empty(); }
  virtual void AppendToText(RegExpText* text, Zone* zone);
  SmartArrayPointer<const char> ToString(Zone* zone);
#define MAKE_ASTYPE(Name)                                                  \
  virtual RegExp##Name* As##Name();                                        \
  virtual bool Is##Name();
  FOR_EACH_REG_EXP_TREE_TYPE(MAKE_ASTYPE)
#undef MAKE_ASTYPE
};


class RegExpDisjunction V8_FINAL : public RegExpTree {
 public:
  explicit RegExpDisjunction(ZoneList<RegExpTree*>* alternatives);
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpDisjunction* AsDisjunction() V8_OVERRIDE;
  virtual Interval CaptureRegisters() V8_OVERRIDE;
  virtual bool IsDisjunction() V8_OVERRIDE;
  virtual bool IsAnchoredAtStart() V8_OVERRIDE;
  virtual bool IsAnchoredAtEnd() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return min_match_; }
  virtual int max_match() V8_OVERRIDE { return max_match_; }
  ZoneList<RegExpTree*>* alternatives() { return alternatives_; }
 private:
  ZoneList<RegExpTree*>* alternatives_;
  int min_match_;
  int max_match_;
};


class RegExpAlternative V8_FINAL : public RegExpTree {
 public:
  explicit RegExpAlternative(ZoneList<RegExpTree*>* nodes);
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpAlternative* AsAlternative() V8_OVERRIDE;
  virtual Interval CaptureRegisters() V8_OVERRIDE;
  virtual bool IsAlternative() V8_OVERRIDE;
  virtual bool IsAnchoredAtStart() V8_OVERRIDE;
  virtual bool IsAnchoredAtEnd() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return min_match_; }
  virtual int max_match() V8_OVERRIDE { return max_match_; }
  ZoneList<RegExpTree*>* nodes() { return nodes_; }
 private:
  ZoneList<RegExpTree*>* nodes_;
  int min_match_;
  int max_match_;
};


class RegExpAssertion V8_FINAL : public RegExpTree {
 public:
  enum AssertionType {
    START_OF_LINE,
    START_OF_INPUT,
    END_OF_LINE,
    END_OF_INPUT,
    BOUNDARY,
    NON_BOUNDARY
  };
  explicit RegExpAssertion(AssertionType type) : assertion_type_(type) { }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpAssertion* AsAssertion() V8_OVERRIDE;
  virtual bool IsAssertion() V8_OVERRIDE;
  virtual bool IsAnchoredAtStart() V8_OVERRIDE;
  virtual bool IsAnchoredAtEnd() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return 0; }
  virtual int max_match() V8_OVERRIDE { return 0; }
  AssertionType assertion_type() { return assertion_type_; }
 private:
  AssertionType assertion_type_;
};


class CharacterSet V8_FINAL BASE_EMBEDDED {
 public:
  explicit CharacterSet(uc16 standard_set_type)
      : ranges_(NULL),
        standard_set_type_(standard_set_type) {}
  explicit CharacterSet(ZoneList<CharacterRange>* ranges)
      : ranges_(ranges),
        standard_set_type_(0) {}
  ZoneList<CharacterRange>* ranges(Zone* zone);
  uc16 standard_set_type() { return standard_set_type_; }
  void set_standard_set_type(uc16 special_set_type) {
    standard_set_type_ = special_set_type;
  }
  bool is_standard() { return standard_set_type_ != 0; }
  void Canonicalize();
 private:
  ZoneList<CharacterRange>* ranges_;
  // If non-zero, the value represents a standard set (e.g., all whitespace
  // characters) without having to expand the ranges.
  uc16 standard_set_type_;
};


class RegExpCharacterClass V8_FINAL : public RegExpTree {
 public:
  RegExpCharacterClass(ZoneList<CharacterRange>* ranges, bool is_negated)
      : set_(ranges),
        is_negated_(is_negated) { }
  explicit RegExpCharacterClass(uc16 type)
      : set_(type),
        is_negated_(false) { }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpCharacterClass* AsCharacterClass() V8_OVERRIDE;
  virtual bool IsCharacterClass() V8_OVERRIDE;
  virtual bool IsTextElement() V8_OVERRIDE { return true; }
  virtual int min_match() V8_OVERRIDE { return 1; }
  virtual int max_match() V8_OVERRIDE { return 1; }
  virtual void AppendToText(RegExpText* text, Zone* zone) V8_OVERRIDE;
  CharacterSet character_set() { return set_; }
  // TODO(lrn): Remove need for complex version if is_standard that
  // recognizes a mangled standard set and just do { return set_.is_special(); }
  bool is_standard(Zone* zone);
  // Returns a value representing the standard character set if is_standard()
  // returns true.
  // Currently used values are:
  // s : unicode whitespace
  // S : unicode non-whitespace
  // w : ASCII word character (digit, letter, underscore)
  // W : non-ASCII word character
  // d : ASCII digit
  // D : non-ASCII digit
  // . : non-unicode non-newline
  // * : All characters
  uc16 standard_type() { return set_.standard_set_type(); }
  ZoneList<CharacterRange>* ranges(Zone* zone) { return set_.ranges(zone); }
  bool is_negated() { return is_negated_; }

 private:
  CharacterSet set_;
  bool is_negated_;
};


class RegExpAtom V8_FINAL : public RegExpTree {
 public:
  explicit RegExpAtom(Vector<const uc16> data) : data_(data) { }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpAtom* AsAtom() V8_OVERRIDE;
  virtual bool IsAtom() V8_OVERRIDE;
  virtual bool IsTextElement() V8_OVERRIDE { return true; }
  virtual int min_match() V8_OVERRIDE { return data_.length(); }
  virtual int max_match() V8_OVERRIDE { return data_.length(); }
  virtual void AppendToText(RegExpText* text, Zone* zone) V8_OVERRIDE;
  Vector<const uc16> data() { return data_; }
  int length() { return data_.length(); }
 private:
  Vector<const uc16> data_;
};


class RegExpText V8_FINAL : public RegExpTree {
 public:
  explicit RegExpText(Zone* zone) : elements_(2, zone), length_(0) {}
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpText* AsText() V8_OVERRIDE;
  virtual bool IsText() V8_OVERRIDE;
  virtual bool IsTextElement() V8_OVERRIDE { return true; }
  virtual int min_match() V8_OVERRIDE { return length_; }
  virtual int max_match() V8_OVERRIDE { return length_; }
  virtual void AppendToText(RegExpText* text, Zone* zone) V8_OVERRIDE;
  void AddElement(TextElement elm, Zone* zone)  {
    elements_.Add(elm, zone);
    length_ += elm.length();
  }
  ZoneList<TextElement>* elements() { return &elements_; }
 private:
  ZoneList<TextElement> elements_;
  int length_;
};


class RegExpQuantifier V8_FINAL : public RegExpTree {
 public:
  enum QuantifierType { GREEDY, NON_GREEDY, POSSESSIVE };
  RegExpQuantifier(int min, int max, QuantifierType type, RegExpTree* body)
      : body_(body),
        min_(min),
        max_(max),
        min_match_(min * body->min_match()),
        quantifier_type_(type) {
    if (max > 0 && body->max_match() > kInfinity / max) {
      max_match_ = kInfinity;
    } else {
      max_match_ = max * body->max_match();
    }
  }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  static RegExpNode* ToNode(int min,
                            int max,
                            bool is_greedy,
                            RegExpTree* body,
                            RegExpCompiler* compiler,
                            RegExpNode* on_success,
                            bool not_at_start = false);
  virtual RegExpQuantifier* AsQuantifier() V8_OVERRIDE;
  virtual Interval CaptureRegisters() V8_OVERRIDE;
  virtual bool IsQuantifier() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return min_match_; }
  virtual int max_match() V8_OVERRIDE { return max_match_; }
  int min() { return min_; }
  int max() { return max_; }
  bool is_possessive() { return quantifier_type_ == POSSESSIVE; }
  bool is_non_greedy() { return quantifier_type_ == NON_GREEDY; }
  bool is_greedy() { return quantifier_type_ == GREEDY; }
  RegExpTree* body() { return body_; }

 private:
  RegExpTree* body_;
  int min_;
  int max_;
  int min_match_;
  int max_match_;
  QuantifierType quantifier_type_;
};


class RegExpCapture V8_FINAL : public RegExpTree {
 public:
  explicit RegExpCapture(RegExpTree* body, int index)
      : body_(body), index_(index) { }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  static RegExpNode* ToNode(RegExpTree* body,
                            int index,
                            RegExpCompiler* compiler,
                            RegExpNode* on_success);
  virtual RegExpCapture* AsCapture() V8_OVERRIDE;
  virtual bool IsAnchoredAtStart() V8_OVERRIDE;
  virtual bool IsAnchoredAtEnd() V8_OVERRIDE;
  virtual Interval CaptureRegisters() V8_OVERRIDE;
  virtual bool IsCapture() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return body_->min_match(); }
  virtual int max_match() V8_OVERRIDE { return body_->max_match(); }
  RegExpTree* body() { return body_; }
  int index() { return index_; }
  static int StartRegister(int index) { return index * 2; }
  static int EndRegister(int index) { return index * 2 + 1; }

 private:
  RegExpTree* body_;
  int index_;
};


class RegExpLookahead V8_FINAL : public RegExpTree {
 public:
  RegExpLookahead(RegExpTree* body,
                  bool is_positive,
                  int capture_count,
                  int capture_from)
      : body_(body),
        is_positive_(is_positive),
        capture_count_(capture_count),
        capture_from_(capture_from) { }

  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpLookahead* AsLookahead() V8_OVERRIDE;
  virtual Interval CaptureRegisters() V8_OVERRIDE;
  virtual bool IsLookahead() V8_OVERRIDE;
  virtual bool IsAnchoredAtStart() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return 0; }
  virtual int max_match() V8_OVERRIDE { return 0; }
  RegExpTree* body() { return body_; }
  bool is_positive() { return is_positive_; }
  int capture_count() { return capture_count_; }
  int capture_from() { return capture_from_; }

 private:
  RegExpTree* body_;
  bool is_positive_;
  int capture_count_;
  int capture_from_;
};


class RegExpBackReference V8_FINAL : public RegExpTree {
 public:
  explicit RegExpBackReference(RegExpCapture* capture)
      : capture_(capture) { }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpBackReference* AsBackReference() V8_OVERRIDE;
  virtual bool IsBackReference() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return 0; }
  virtual int max_match() V8_OVERRIDE { return capture_->max_match(); }
  int index() { return capture_->index(); }
  RegExpCapture* capture() { return capture_; }
 private:
  RegExpCapture* capture_;
};


class RegExpEmpty V8_FINAL : public RegExpTree {
 public:
  RegExpEmpty() { }
  virtual void* Accept(RegExpVisitor* visitor, void* data) V8_OVERRIDE;
  virtual RegExpNode* ToNode(RegExpCompiler* compiler,
                             RegExpNode* on_success) V8_OVERRIDE;
  virtual RegExpEmpty* AsEmpty() V8_OVERRIDE;
  virtual bool IsEmpty() V8_OVERRIDE;
  virtual int min_match() V8_OVERRIDE { return 0; }
  virtual int max_match() V8_OVERRIDE { return 0; }
  static RegExpEmpty* GetInstance() {
    static RegExpEmpty* instance = ::new RegExpEmpty();
    return instance;
  }
};


// ----------------------------------------------------------------------------
// Out-of-line inline constructors (to side-step cyclic dependencies).

inline ModuleVariable::ModuleVariable(Zone* zone, VariableProxy* proxy, int pos)
    : Module(zone, proxy->interface(), pos),
      proxy_(proxy) {
}


// ----------------------------------------------------------------------------
// Basic visitor
// - leaf node visitors are abstract.

class AstVisitor BASE_EMBEDDED {
 public:
  AstVisitor() {}
  virtual ~AstVisitor() {}

  // Stack overflow check and dynamic dispatch.
  virtual void Visit(AstNode* node) = 0;

  // Iteration left-to-right.
  virtual void VisitDeclarations(ZoneList<Declaration*>* declarations);
  virtual void VisitStatements(ZoneList<Statement*>* statements);
  virtual void VisitExpressions(ZoneList<Expression*>* expressions);

  // Individual AST nodes.
#define DEF_VISIT(type)                         \
  virtual void Visit##type(type* node) = 0;
  AST_NODE_LIST(DEF_VISIT)
#undef DEF_VISIT
};


#define DEFINE_AST_VISITOR_SUBCLASS_MEMBERS()                       \
public:                                                             \
  virtual void Visit(AstNode* node) V8_FINAL V8_OVERRIDE {          \
    if (!CheckStackOverflow()) node->Accept(this);                  \
  }                                                                 \
                                                                    \
  void SetStackOverflow() { stack_overflow_ = true; }               \
  void ClearStackOverflow() { stack_overflow_ = false; }            \
  bool HasStackOverflow() const { return stack_overflow_; }         \
                                                                    \
  bool CheckStackOverflow() {                                       \
    if (stack_overflow_) return true;                               \
    StackLimitCheck check(zone_->isolate());                        \
    if (!check.HasOverflowed()) return false;                       \
    return (stack_overflow_ = true);                                \
  }                                                                 \
                                                                    \
private:                                                            \
  void InitializeAstVisitor(Zone* zone) {                           \
    zone_ = zone;                                                   \
    stack_overflow_ = false;                                        \
  }                                                                 \
  Zone* zone() { return zone_; }                                    \
  Isolate* isolate() { return zone_->isolate(); }                   \
                                                                    \
  Zone* zone_;                                                      \
  bool stack_overflow_


// ----------------------------------------------------------------------------
// Construction time visitor.

class AstConstructionVisitor BASE_EMBEDDED {
 public:
  AstConstructionVisitor() : dont_optimize_reason_(kNoReason) { }

  AstProperties* ast_properties() { return &properties_; }
  BailoutReason dont_optimize_reason() { return dont_optimize_reason_; }

 private:
  template<class> friend class AstNodeFactory;

  // Node visitors.
#define DEF_VISIT(type) \
  void Visit##type(type* node);
  AST_NODE_LIST(DEF_VISIT)
#undef DEF_VISIT

  void increase_node_count() { properties_.add_node_count(1); }
  void add_flag(AstPropertiesFlag flag) { properties_.flags()->Add(flag); }
  void set_dont_optimize_reason(BailoutReason reason) {
      dont_optimize_reason_ = reason;
  }

  void add_slot_node(FeedbackSlotInterface* slot_node) {
    int count = slot_node->ComputeFeedbackSlotCount();
    if (count > 0) {
      slot_node->SetFirstFeedbackSlot(properties_.feedback_slots());
      properties_.increase_feedback_slots(count);
    }
  }

  AstProperties properties_;
  BailoutReason dont_optimize_reason_;
};


class AstNullVisitor BASE_EMBEDDED {
 public:
  // Node visitors.
#define DEF_VISIT(type) \
  void Visit##type(type* node) {}
  AST_NODE_LIST(DEF_VISIT)
#undef DEF_VISIT
};



// ----------------------------------------------------------------------------
// AstNode factory

template<class Visitor>
class AstNodeFactory V8_FINAL BASE_EMBEDDED {
 public:
  explicit AstNodeFactory(Zone* zone) : zone_(zone) { }

  Visitor* visitor() { return &visitor_; }

#define VISIT_AND_RETURN(NodeType, node) \
  visitor_.Visit##NodeType((node)); \
  return node;

  VariableDeclaration* NewVariableDeclaration(VariableProxy* proxy,
                                              VariableMode mode,
                                              Scope* scope,
                                              int pos) {
    VariableDeclaration* decl =
        new(zone_) VariableDeclaration(zone_, proxy, mode, scope, pos);
    VISIT_AND_RETURN(VariableDeclaration, decl)
  }

  FunctionDeclaration* NewFunctionDeclaration(VariableProxy* proxy,
                                              VariableMode mode,
                                              FunctionLiteral* fun,
                                              Scope* scope,
                                              int pos) {
    FunctionDeclaration* decl =
        new(zone_) FunctionDeclaration(zone_, proxy, mode, fun, scope, pos);
    VISIT_AND_RETURN(FunctionDeclaration, decl)
  }

  ModuleDeclaration* NewModuleDeclaration(VariableProxy* proxy,
                                          Module* module,
                                          Scope* scope,
                                          int pos) {
    ModuleDeclaration* decl =
        new(zone_) ModuleDeclaration(zone_, proxy, module, scope, pos);
    VISIT_AND_RETURN(ModuleDeclaration, decl)
  }

  ImportDeclaration* NewImportDeclaration(VariableProxy* proxy,
                                          Module* module,
                                          Scope* scope,
                                          int pos) {
    ImportDeclaration* decl =
        new(zone_) ImportDeclaration(zone_, proxy, module, scope, pos);
    VISIT_AND_RETURN(ImportDeclaration, decl)
  }

  ExportDeclaration* NewExportDeclaration(VariableProxy* proxy,
                                          Scope* scope,
                                          int pos) {
    ExportDeclaration* decl =
        new(zone_) ExportDeclaration(zone_, proxy, scope, pos);
    VISIT_AND_RETURN(ExportDeclaration, decl)
  }

  ModuleLiteral* NewModuleLiteral(Block* body, Interface* interface, int pos) {
    ModuleLiteral* module =
        new(zone_) ModuleLiteral(zone_, body, interface, pos);
    VISIT_AND_RETURN(ModuleLiteral, module)
  }

  ModuleVariable* NewModuleVariable(VariableProxy* proxy, int pos) {
    ModuleVariable* module = new(zone_) ModuleVariable(zone_, proxy, pos);
    VISIT_AND_RETURN(ModuleVariable, module)
  }

  ModulePath* NewModulePath(Module* origin, Handle<String> name, int pos) {
    ModulePath* module = new(zone_) ModulePath(zone_, origin, name, pos);
    VISIT_AND_RETURN(ModulePath, module)
  }

  ModuleUrl* NewModuleUrl(Handle<String> url, int pos) {
    ModuleUrl* module = new(zone_) ModuleUrl(zone_, url, pos);
    VISIT_AND_RETURN(ModuleUrl, module)
  }

  Block* NewBlock(ZoneStringList* labels,
                  int capacity,
                  bool is_initializer_block,
                  int pos) {
    Block* block = new(zone_) Block(
        zone_, labels, capacity, is_initializer_block, pos);
    VISIT_AND_RETURN(Block, block)
  }

#define STATEMENT_WITH_LABELS(NodeType) \
  NodeType* New##NodeType(ZoneStringList* labels, int pos) { \
    NodeType* stmt = new(zone_) NodeType(zone_, labels, pos); \
    VISIT_AND_RETURN(NodeType, stmt); \
  }
  STATEMENT_WITH_LABELS(DoWhileStatement)
  STATEMENT_WITH_LABELS(WhileStatement)
  STATEMENT_WITH_LABELS(ForStatement)
  STATEMENT_WITH_LABELS(SwitchStatement)
#undef STATEMENT_WITH_LABELS

  ForEachStatement* NewForEachStatement(ForEachStatement::VisitMode visit_mode,
                                        ZoneStringList* labels,
                                        int pos) {
    switch (visit_mode) {
      case ForEachStatement::ENUMERATE: {
        ForInStatement* stmt = new(zone_) ForInStatement(zone_, labels, pos);
        VISIT_AND_RETURN(ForInStatement, stmt);
      }
      case ForEachStatement::ITERATE: {
        ForOfStatement* stmt = new(zone_) ForOfStatement(zone_, labels, pos);
        VISIT_AND_RETURN(ForOfStatement, stmt);
      }
    }
    UNREACHABLE();
    return NULL;
  }

  ModuleStatement* NewModuleStatement(
      VariableProxy* proxy, Block* body, int pos) {
    ModuleStatement* stmt = new(zone_) ModuleStatement(zone_, proxy, body, pos);
    VISIT_AND_RETURN(ModuleStatement, stmt)
  }

  ExpressionStatement* NewExpressionStatement(Expression* expression, int pos) {
    ExpressionStatement* stmt =
        new(zone_) ExpressionStatement(zone_, expression, pos);
    VISIT_AND_RETURN(ExpressionStatement, stmt)
  }

  ContinueStatement* NewContinueStatement(IterationStatement* target, int pos) {
    ContinueStatement* stmt = new(zone_) ContinueStatement(zone_, target, pos);
    VISIT_AND_RETURN(ContinueStatement, stmt)
  }

  BreakStatement* NewBreakStatement(BreakableStatement* target, int pos) {
    BreakStatement* stmt = new(zone_) BreakStatement(zone_, target, pos);
    VISIT_AND_RETURN(BreakStatement, stmt)
  }

  ReturnStatement* NewReturnStatement(Expression* expression, int pos) {
    ReturnStatement* stmt = new(zone_) ReturnStatement(zone_, expression, pos);
    VISIT_AND_RETURN(ReturnStatement, stmt)
  }

  WithStatement* NewWithStatement(Scope* scope,
                                  Expression* expression,
                                  Statement* statement,
                                  int pos) {
    WithStatement* stmt = new(zone_) WithStatement(
        zone_, scope, expression, statement, pos);
    VISIT_AND_RETURN(WithStatement, stmt)
  }

  IfStatement* NewIfStatement(Expression* condition,
                              Statement* then_statement,
                              Statement* else_statement,
                              int pos) {
    IfStatement* stmt = new(zone_) IfStatement(
        zone_, condition, then_statement, else_statement, pos);
    VISIT_AND_RETURN(IfStatement, stmt)
  }

  TryCatchStatement* NewTryCatchStatement(int index,
                                          Block* try_block,
                                          Scope* scope,
                                          Variable* variable,
                                          Block* catch_block,
                                          int pos) {
    TryCatchStatement* stmt = new(zone_) TryCatchStatement(
        zone_, index, try_block, scope, variable, catch_block, pos);
    VISIT_AND_RETURN(TryCatchStatement, stmt)
  }

  TryFinallyStatement* NewTryFinallyStatement(int index,
                                              Block* try_block,
                                              Block* finally_block,
                                              int pos) {
    TryFinallyStatement* stmt = new(zone_) TryFinallyStatement(
        zone_, index, try_block, finally_block, pos);
    VISIT_AND_RETURN(TryFinallyStatement, stmt)
  }

  DebuggerStatement* NewDebuggerStatement(int pos) {
    DebuggerStatement* stmt = new(zone_) DebuggerStatement(zone_, pos);
    VISIT_AND_RETURN(DebuggerStatement, stmt)
  }

  EmptyStatement* NewEmptyStatement(int pos) {
    return new(zone_) EmptyStatement(zone_, pos);
  }

  CaseClause* NewCaseClause(
      Expression* label, ZoneList<Statement*>* statements, int pos) {
    CaseClause* clause =
        new(zone_) CaseClause(zone_, label, statements, pos);
    VISIT_AND_RETURN(CaseClause, clause)
  }

  Literal* NewLiteral(Handle<Object> handle, int pos) {
    Literal* lit = new(zone_) Literal(zone_, handle, pos);
    VISIT_AND_RETURN(Literal, lit)
  }

  Literal* NewNumberLiteral(double number, int pos) {
    return NewLiteral(
        zone_->isolate()->factory()->NewNumber(number, TENURED), pos);
  }

  ObjectLiteral* NewObjectLiteral(
      ZoneList<ObjectLiteral::Property*>* properties,
      int literal_index,
      int boilerplate_properties,
      bool has_function,
      int pos) {
    ObjectLiteral* lit = new(zone_) ObjectLiteral(
        zone_, properties, literal_index, boilerplate_properties,
        has_function, pos);
    VISIT_AND_RETURN(ObjectLiteral, lit)
  }

  ObjectLiteral::Property* NewObjectLiteralProperty(Literal* key,
                                                    Expression* value) {
    return new(zone_) ObjectLiteral::Property(zone_, key, value);
  }

  ObjectLiteral::Property* NewObjectLiteralProperty(bool is_getter,
                                                    FunctionLiteral* value,
                                                    int pos) {
    ObjectLiteral::Property* prop =
        new(zone_) ObjectLiteral::Property(zone_, is_getter, value);
    prop->set_key(NewLiteral(value->name(), pos));
    return prop;  // Not an AST node, will not be visited.
  }

  RegExpLiteral* NewRegExpLiteral(Handle<String> pattern,
                                  Handle<String> flags,
                                  int literal_index,
                                  int pos) {
    RegExpLiteral* lit =
        new(zone_) RegExpLiteral(zone_, pattern, flags, literal_index, pos);
    VISIT_AND_RETURN(RegExpLiteral, lit);
  }

  ArrayLiteral* NewArrayLiteral(ZoneList<Expression*>* values,
                                int literal_index,
                                int pos) {
    ArrayLiteral* lit = new(zone_) ArrayLiteral(
        zone_, values, literal_index, pos);
    VISIT_AND_RETURN(ArrayLiteral, lit)
  }

  VariableProxy* NewVariableProxy(Variable* var,
                                  int pos = RelocInfo::kNoPosition) {
    VariableProxy* proxy = new(zone_) VariableProxy(zone_, var, pos);
    VISIT_AND_RETURN(VariableProxy, proxy)
  }

  VariableProxy* NewVariableProxy(Handle<String> name,
                                  bool is_this,
                                  Interface* interface = Interface::NewValue(),
                                  int position = RelocInfo::kNoPosition) {
    VariableProxy* proxy =
        new(zone_) VariableProxy(zone_, name, is_this, interface, position);
    VISIT_AND_RETURN(VariableProxy, proxy)
  }

  Property* NewProperty(Expression* obj, Expression* key, int pos) {
    Property* prop = new(zone_) Property(zone_, obj, key, pos);
    VISIT_AND_RETURN(Property, prop)
  }

  Call* NewCall(Expression* expression,
                ZoneList<Expression*>* arguments,
                int pos) {
    Call* call = new(zone_) Call(zone_, expression, arguments, pos);
    VISIT_AND_RETURN(Call, call)
  }

  CallNew* NewCallNew(Expression* expression,
                      ZoneList<Expression*>* arguments,
                      int pos) {
    CallNew* call = new(zone_) CallNew(zone_, expression, arguments, pos);
    VISIT_AND_RETURN(CallNew, call)
  }

  CallRuntime* NewCallRuntime(Handle<String> name,
                              const Runtime::Function* function,
                              ZoneList<Expression*>* arguments,
                              int pos) {
    CallRuntime* call =
        new(zone_) CallRuntime(zone_, name, function, arguments, pos);
    VISIT_AND_RETURN(CallRuntime, call)
  }

  UnaryOperation* NewUnaryOperation(Token::Value op,
                                    Expression* expression,
                                    int pos) {
    UnaryOperation* node =
        new(zone_) UnaryOperation(zone_, op, expression, pos);
    VISIT_AND_RETURN(UnaryOperation, node)
  }

  BinaryOperation* NewBinaryOperation(Token::Value op,
                                      Expression* left,
                                      Expression* right,
                                      int pos) {
    BinaryOperation* node =
        new(zone_) BinaryOperation(zone_, op, left, right, pos);
    VISIT_AND_RETURN(BinaryOperation, node)
  }

  CountOperation* NewCountOperation(Token::Value op,
                                    bool is_prefix,
                                    Expression* expr,
                                    int pos) {
    CountOperation* node =
        new(zone_) CountOperation(zone_, op, is_prefix, expr, pos);
    VISIT_AND_RETURN(CountOperation, node)
  }

  CompareOperation* NewCompareOperation(Token::Value op,
                                        Expression* left,
                                        Expression* right,
                                        int pos) {
    CompareOperation* node =
        new(zone_) CompareOperation(zone_, op, left, right, pos);
    VISIT_AND_RETURN(CompareOperation, node)
  }

  Conditional* NewConditional(Expression* condition,
                              Expression* then_expression,
                              Expression* else_expression,
                              int position) {
    Conditional* cond = new(zone_) Conditional(
        zone_, condition, then_expression, else_expression, position);
    VISIT_AND_RETURN(Conditional, cond)
  }

  Assignment* NewAssignment(Token::Value op,
                            Expression* target,
                            Expression* value,
                            int pos) {
    Assignment* assign =
        new(zone_) Assignment(zone_, op, target, value, pos);
    assign->Init(zone_, this);
    VISIT_AND_RETURN(Assignment, assign)
  }

  Yield* NewYield(Expression *generator_object,
                  Expression* expression,
                  Yield::Kind yield_kind,
                  int pos) {
    Yield* yield = new(zone_) Yield(
        zone_, generator_object, expression, yield_kind, pos);
    VISIT_AND_RETURN(Yield, yield)
  }

  Throw* NewThrow(Expression* exception, int pos) {
    Throw* t = new(zone_) Throw(zone_, exception, pos);
    VISIT_AND_RETURN(Throw, t)
  }

  FunctionLiteral* NewFunctionLiteral(
      Handle<String> name,
      Scope* scope,
      ZoneList<Statement*>* body,
      int materialized_literal_count,
      int expected_property_count,
      int handler_count,
      int parameter_count,
      FunctionLiteral::ParameterFlag has_duplicate_parameters,
      FunctionLiteral::FunctionType function_type,
      FunctionLiteral::IsFunctionFlag is_function,
      FunctionLiteral::IsParenthesizedFlag is_parenthesized,
      FunctionLiteral::IsGeneratorFlag is_generator,
      int position) {
    FunctionLiteral* lit = new(zone_) FunctionLiteral(
        zone_, name, scope, body,
        materialized_literal_count, expected_property_count, handler_count,
        parameter_count, function_type, has_duplicate_parameters, is_function,
        is_parenthesized, is_generator, position);
    // Top-level literal doesn't count for the AST's properties.
    if (is_function == FunctionLiteral::kIsFunction) {
      visitor_.VisitFunctionLiteral(lit);
    }
    return lit;
  }

  NativeFunctionLiteral* NewNativeFunctionLiteral(
      Handle<String> name, v8::Extension* extension, int pos) {
    NativeFunctionLiteral* lit =
        new(zone_) NativeFunctionLiteral(zone_, name, extension, pos);
    VISIT_AND_RETURN(NativeFunctionLiteral, lit)
  }

  ThisFunction* NewThisFunction(int pos) {
    ThisFunction* fun = new(zone_) ThisFunction(zone_, pos);
    VISIT_AND_RETURN(ThisFunction, fun)
  }

#undef VISIT_AND_RETURN

 private:
  Zone* zone_;
  Visitor visitor_;
};


} }  // namespace v8::internal

#endif  // V8_AST_H_
