/**
 * Macro to include in the public part of a tree element that is supposed to be 
 * visitibla by a static tree visitor, e.g.,:
 *
 *   class ConcreteElement {
 *
 *   public:
 *
 *     YANTA_TREE_VISITABLE();
 *   };
 *
 * Ensures that the correct visitor method for the type of the element is 
 * called.
 */
#define YANTA_TREE_VISITABLE() template <typename VisitorType> void accept(VisitorType& visitor) { visitor.visit(*this); traverse(visitor); }
