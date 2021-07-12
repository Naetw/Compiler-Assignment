# Visitor Pattern

**Example**

A simple example is available in `example/`. You can use `make` to build it. The example contains some implementation details, which you may be interested in.

## Why do we use Visitor Pattern?

In this assignment, we only need to perform a single task against the AST, which is dumping node information. We expect you to design a base class for the AST node and then derive different node classes from the base class according to different language constructs. Afterward, you only have to build the AST and then traverse it to dump the information of each node.
So it seems you don't really need to use Visitor Pattern to finish this assignment. However, we need to consider maintainability when developing software.

Let's get back to the goal of this semester - **building a compiler**. This implies we will need to perform other tasks with the AST later besides dumping node information.
For example, we may need to perform semantic analysis, optimizations, and code generation.
If we use polymorphism to design an API for each task (e.g., `print` or `codegen`), it will lengthen and complicate the class of module of each node. However, "**each task has only one in common - performing a task against the AST**". This design leads to poor maintainability since the developers have to spend some time thinking about what's the relationship among these APIs and find out they have independent functionalities in the end.

In OOP design, there is an acronym called "[SOLID](https://en.wikipedia.org/wiki/SOLID)" for five principles, the first one of which (S) stands for **Single Responsibility Principle (SRP)**. In the situation aforementioned, it violates this principle. Therefore, we hope to make the AST-node-related class do only one thing - "preserving the program structure"; as for other tasks, we use other modules for extension, rather than extending each node class directly.

Visitor Pattern is really suitable for this situation. The core concept is to deal with "operations" and "data" separately, which matches the principle aforementioned: classes for AST nodes are responsible for preserving program structure (data), while different visitors are responsible for dumping information (in this case), semantic analysis, optimization, or code generation (operation).

In fact, just using polymorphism is more reasonable in this assignment, after all, `print` is to dump the information, which does not violate the principle of SRP. However, considering you may be quite busy during the next assignment, we suggest you use Visitor Pattern for this assignment. As a consequence, you need to only  focus on the implementation of the functionalities and don't need to worry about the design pattern in the next assignment.

## Visitor Pattern

Visitor Pattern is a design pattern. In this section, we will introduce what it is.

> :raised_hand: VP denotes the visitor pattern henceforth.

**First class of Visitor Pattern**

Here is a scenario: there are some shops on a street, like a toy store or a book store. There are some customers: children, and adults. Different people would behave differently in different stores.

- Children
  - Toy Store: Buy Lego
  - Book Store: Buy [JavaScript for Kids](https://www.reddit.com/r/ProgrammerHumor/comments/8uuw4g/javascript_for_kids/)
- Adults
  - Toy Store: Buy models
  - Book Store: Buy [C++ Programming through Memes](https://www.goodreads.com/book/show/49749850-c-programming-through-memes)

If you write the code in the polymorphism manner, the code may look like this:

```cpp
// In ToyStore.hpp
class ToyStore : public Store {
  public:
    void visitedByChild(Child &child) override {
        std::cout << child.name() << " buys some Lego\n";
        sell("Lego");
    }

    void visitedByAdult(Adult &adult) override {
        std::cout << adult.name() << " buys some models\n";
        sell("model");
    }
};

// In BookStore.hpp
class BookStore : public Store {
  public:
    void visitedByChild(Child &child) override {
        std::cout << child.name() << " buys \"JavaScript for Kids\"\n";
        sell("JavaScript for Kids");
    }

    void visitedByAdult(Adult &adult) override {
        std::cout << adult.name() << " buys \"C++ Programming through Memes\"\n";
        sell("C++ Programming through Memes");
    }
};
```

You can see that these member functions have little to do with the class and each other function, even the operation itself is not decided by the class ("visitors" buy stuff). Here we can use VP to refactor the code: putting the behaviors of children and adults together, and making stores become pure data providers.

```cpp
// In ChildVisitor.hpp
class ChildVisitor : public StoreVisitorBase {
  public:
    void visit(ToyStore &toy_store) override {
        std::cout << name << " buys some Lego\n";
        toy_store.sell("Lego");
    }

    void visit(BookStore &book_store) override {
        std::cout << name << " buys \"JavaScript for Kids\"\n";
        book_store.sell("JavaScript for Kids");
    }
};

// In AdultVisitor.hpp
class AdultVisitor : public StoreVisitorBase {
  public:
    void visit(ToyStore &toy_store) override {
        std::cout << name << " buys some models\n";
        toy_store.sell("model");
    }

    void visit(BookStore &book_store) override {
        std::cout << name << " buys \"C++ Programming through Memes\"\n";
        book_store.sell("C++ Programming through Memes");
    }
};
```

Here we have written two visitors, each of them is responsible for specific operations. The next problem is how to call the corresponding `visit` member function correctly?

In function overloading, the effective function got called is determined by the compiler in compile-time based on the type of actual parameters. Therefore, the scenario of calling functions maybe like this:

```cpp
ChildVisitor cv;

ToyStore ts(); // arguments omitted for simplicity
BookStore bs(); // arguments omitted for simplicity

cv.visit(ts);
cv.visit(bs);
```

Now, assuming a weird scenario: we go to a street where the stores on this street have no signboard and we don't know what kind of store they are:

```cpp
std::vector <Store *> store_list = { s1, s2, s3 };
```

In this scenario, the compiler determine which `visit` function should be called correctly if we use `visit` function to take each element (store) as the actual parameter could not make. This is because, for the compiler, the types of the actual parameter are all `Store *`. Then, how to get the derived class from the pointer to the base class (`Store *`)?

Remind that the polymorphism in the assignment spec: "a call to a member function will cause a different function to be executed, depending on the real type of the object that calls the function", we can take advantage of virtual functions to call different functions; besides, there is an implicit formal parameter - `this` pointer in the member function in C++, which points to the object itself. As a result, we can get the derived class pointer in some virtual member function and pass that pointer into `visit` function of a visitor.


```cpp
// In Store.hpp
class Store {
  public:
    virtual void accept(StoreVisitorBase &v) = 0; // pure virtual function
};

// In BookStore.hpp
class BookStore : public Store {
  public:
    void accept(StoreVisitorBase &v) override {
        v->visit(*this); // typeof(this) == BookStore *, calling visit(BookStore *book_store)
    }
};

// In main.cpp
ChildVisitor cv;
std::vector <Store *> store_list = { s1, s2, s3 };

for (Store *store : store_list) {
  store->accept(cv);
}
```

Assumes that s1 is a book store. Although the type of the `store` variable is `Store *` in the loop, we can still call the implementation in `BookStore.hpp` through calling the virtual function `store->accept()`. The compiler can determine which version of `visit` function should be called in that implementation due to `this` pointer.

In the end, let's summarize the whole scenario of this example (the content in parentheses corresponds to AST in the assignment):

1. Separate "data" and "operations" - retrieve the same kind of operations and make them a visitor.
    
    First, we have many stores (AST nodes), and the products in each store may be different (an AST node may be `Ifnode`, `AssignmentNode`, etc.). We want the stores to just take care of the products (AST nodes just preserve the program structure), and deal with the behavior of the visitor separately. 

2. Make a data provider provide an interface for visitors to recognize the real class and get the proper data:
    
    Declare a pure virtual function (`accept`) in the base class of stores (base class of AST nodes), and the implementation is provided by the different stores (different types of AST nodes). The content of implementation is to provide the type of store (the type of AST node) - `this` pointer to visitors basically.
    
**Drawbacks of Visitor Pattern**

Consider this situation:

>  `Konpilor` is a warrior who studies compiler for a long time. In recent years, he dedicated to develop a great compiler, called Life Companion Compiler (**LCC**). You need to only describe the characteristics of your better half, and `LCC` will compile the perfect companion for you. After implementing and optimizing for 5000 different classes of visitors... Oh no! He forgot the description of `brain` in AST nodes.
> 
>  `Konpilor` is a practical person, he cannot tolerate his companion is brainless, while he can't tolerate having no companion either. So the only choice is to write the visit member functions of `brain` for each visitor class.

In the situation above, if you want to add a new node type after writing a bunch of `visitor` classes, the workload would be horrible. To support the `brain` class in each `visitor class` means you need to write 5000 funcitons.

Fortunately, in this course, there won't be this kind of problem, and the reason is simple: *~~you don't need a LCC and still can find your companion~~* the type of AST nodes are fixed and won't be exteneded after Assignment III. Under this circumstance, using `Visitor Pattern` is perfect ideally.

One more thing to mention is that most of the modern compilers adapt `Visitor Pattern` in their implementation. So, we can say that it's a classic design pattern of designing a compiler. After learning this concept, your project is closer to the real world. If there is sample code or exams in the future, we will assume that all of you have understood `VP`. :heart:
