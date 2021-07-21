/***************************************************************************//**
 * @file cmiss_btree.hpp
 *
 * N-way binary tree container template class.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_BTREE_HPP)
#define CMZN_BTREE_HPP

#include <functional>

template<typename object_type, typename identifier_type, int B_TREE_ORDER = 5,
	typename _Compare = std::less<identifier_type> > class cmzn_btree
{
private:
	typedef int CONDITIONAL_FUNCTION(object_type *object,void *user_data);
	typedef int ITERATOR_FUNCTION(object_type *object,void *user_data);

	class INDEX_NODE
	{
	private:
		int number_of_indices;
		// break point indices (identifiers accessed through objects)
		// only leaf nodes access objects
		object_type *indices[2*B_TREE_ORDER];
		INDEX_NODE *parent;
		INDEX_NODE **children;

		INDEX_NODE(bool leaf) :
			number_of_indices(0),
			parent(0),
			children(leaf ? 0 : new INDEX_NODE*[2*B_TREE_ORDER + 1])
		{
		}

		/** deep copy constructor */
		INDEX_NODE(const INDEX_NODE &source) :
			number_of_indices(source.number_of_indices),
			parent(0),
			children(source.children ? new INDEX_NODE*[2*B_TREE_ORDER + 1] : 0)
		{
			bool leaf = !children;
			object_type * const *source_index = source.indices;
			object_type **duplicate_index = indices;
			if (leaf)
			{
				for (int i = number_of_indices; i > 0; --i)
				{
					*duplicate_index = (*source_index)->access();
					source_index++;
					duplicate_index++;
				}
			}
			else
			{
				INDEX_NODE **source_child = source.children;
				INDEX_NODE **duplicate_child = children;
				for (int i = number_of_indices; i >= 0; --i)
				{
					*duplicate_child = new INDEX_NODE(**source_child);
					(*duplicate_child)->parent = this;
					source_child++;
					duplicate_child++;
					if (i)
					{
						*duplicate_index= *source_index;
						source_index++;
						duplicate_index++;
					}
				}
			}
		}

	public:

		~INDEX_NODE()
		{
			if (children)
			{
				for (int i = 0; i <= number_of_indices; ++i)
				{
					delete children[i];
				}
				delete[] children;
			}
			else
			{
				// only leaf nodes access objects
				for (int i = 0; i < number_of_indices; ++i)
				{
					indices[i]->deaccess(&indices[i]);
				}
			}
		}

		static inline INDEX_NODE *createLeaf()
		{
			return new INDEX_NODE(true);
		}

		static inline INDEX_NODE *createStem()
		{
			return new INDEX_NODE(false);
		}

		INDEX_NODE *duplicate() const
		{
			return new INDEX_NODE(*this);
		}

		/** Finds the leaf node that should contain the object with the specified identifier */
		INDEX_NODE *FIND_LEAF_NODE_IN_INDEX(identifier_type identifier)
		{
			if (children)
			{
				INDEX_NODE **child = children + number_of_indices;
				int i = number_of_indices;
				object_type **object_index = indices + (number_of_indices - 1);
				_Compare compare;
				while ((i > 0) && !compare((*object_index)->get_identifier(), identifier))
				{
					--object_index;
					--child;
					--i;
				}
				return (*child)->FIND_LEAF_NODE_IN_INDEX(identifier);
			}
			return this;
		}

		/** Removes an <object> from the <index>. */
		static int REMOVE_OBJECT_FROM_INDEX(object_type *object, INDEX_NODE **index_address)
		{
			int return_code = 0;
			if (index_address && object)
			{
				INDEX_NODE *child, *index;
				if (0 != (index= *index_address))
				{
					/* find the object within the node */
					int i = 0;
					identifier_type identifier = object->get_identifier();
					_Compare compare;
					while ((i < index->number_of_indices) && compare((index->indices)[i]->get_identifier(), identifier))
					{
						i++;
					}
					if (index->children)
					{
						if (0 != (return_code = REMOVE_OBJECT_FROM_INDEX(object,
							&(index->children[i]))))
						{
							/* shuffle down any NULL children */
							if (!(index->children[i]))
							{
								(index->number_of_indices)--;
								for (int j = i; j <= index->number_of_indices; ++j)
								{
									if (j < index->number_of_indices)
									{
										index->indices[j] = index->indices[j + 1];
									}
									index->children[j] = index->children[j + 1];
								}
							}
							if (0 == index->number_of_indices)
							{
								/* one child - chain from parent */
								/* first give it its parents parent */
								index->children[0]->parent = index->parent;
								/* now chain from parent */
								*index_address = index->children[0];
								/* ensure the destroy does not chain */
								index->number_of_indices--;
								/* this next line is redundant */
								index->children[0] = 0;
								delete index;
								index = 0;
							}
							else
							{
								/* make sure that not using a removed object as an index */
								if ((i<index->number_of_indices)&&(object==(index->indices)[i]))
								{
									/* find the last object in the child */
									child=(index->children)[i];
									while (child->children)
									{
										child=(child->children)[child->number_of_indices];
									}
									index->indices[i]=
										(child->indices)[(child->number_of_indices)-1];
								}
							}
						}
					}
					else
					{
						if ((i < index->number_of_indices) && (object == (index->indices)[i]))
						{
							index->indices[i]->deaccess(&(index->indices[i]));
							(index->number_of_indices)--;
							if (0==index->number_of_indices)
							{
								delete *index_address;
								*index_address = 0;
							}
							else
							{
								for (int j = i; j < index->number_of_indices; ++j)
								{
									index->indices[j] = index->indices[j + 1];
								}
							}
							return_code=1;
						}
					}
				}
			}
			else
			{
				//display_message(ERROR_MESSAGE, "REMOVE_OBJECT_FROM_INDEX.  Invalid argument(s)");
			}
			return (return_code);
		}

		/** Removes each object that <iterator> returns true for. */
		static int REMOVE_OBJECTS_FROM_INDEX_THAT(CONDITIONAL_FUNCTION *conditional,
			void *user_data, INDEX_NODE **index_address)
		{
			int count = 0;
			/* check the arguments */
			if (index_address && conditional)
			{
				int i, j, original_number_of_indices;
				INDEX_NODE *child, *index;
				if (0 != (index= *index_address))
				{
					if (index->children)
					{
						original_number_of_indices = index->number_of_indices;
						for (i = 0; i <= original_number_of_indices; i++)
						{
							/* Note indices partition children, hence one fewer */
							if (i < original_number_of_indices)
							{
								/* temporarily ACCESS indices in case objects removed. Needed for
									 conditional test below */
								index->indices[i]->access();
							}
							count += REMOVE_OBJECTS_FROM_INDEX_THAT(
								conditional,user_data,&(index->children[i]));
						}
						j=0;
						/* shuffle down any NULL children */
						for (i = 0; i <= original_number_of_indices;i++)
						{
							if (index->children[i])
							{
								if (i < original_number_of_indices)
								{
									index->indices[j]=index->indices[i];
								}
								index->children[j]=index->children[i];
								j++;
							}
							else if (i < original_number_of_indices)
							{
								/* must undo temporary ACCESS for removed NULL child indices */
								index->indices[i]->deaccess(&(index->indices[i]));
							}
							else if (0 < j)
							{
								/* must undo temporary ACCESS for removed NULL child indices */
								index->indices[j - 1]->deaccess(&(index->indices[j - 1]));
							}
						}
						index->number_of_indices=j-1;
						if (-1==index->number_of_indices)
						{
							/* no children left */
							delete *index_address;
							*index_address = 0;
						}
						else if (0==index->number_of_indices)
						{
							/* one child - chain from parent */
							/* first give it its parents parent */
							index->children[0]->parent=index->parent;
							/* now chain from parent */
							*index_address=index->children[0];
							/* ensure the destroy does not chain */
							index->number_of_indices--;
							/* this next line is redundant */
							index->children[0] = 0;
							delete index;
							index = 0;
						}
						else
						{
							/* make sure that not using a removed object as an index */
							for (i = 0; i < index->number_of_indices; i++)
							{
								if ((conditional)(index->indices[i],user_data))
								{
									/* undo temporary ACCESS for removed NULL child indices */
									index->indices[i]->deaccess(&(index->indices[i]));
									/* find the last object in the child */
									child=(index->children)[i];
									while (child->children)
									{
										child=(child->children)[child->number_of_indices];
									}
									index->indices[i]=
										(child->indices)[(child->number_of_indices)-1];
								}
								else
								{
									/* undo temporary ACCESS for removed NULL child indices
										 copy and deaccess so pointer not cleared */
									object_type *temp_object = index->indices[i];
									temp_object->deaccess(&temp_object);
								}
							}
						}
					}
					else
					{
						/* loop over all indices */
						j=0;
						for (i=0;i<index->number_of_indices;i++)
						{
							index->indices[j]=index->indices[i];
							if ((conditional)(index->indices[j],user_data))
							{
								/* delete the object */
								count++;
								index->indices[j]->deaccess(&(index->indices[j]));
							}
							else
							{
								j++;
							}
						}
						index->number_of_indices=j;
						/* have we killed the node */
						if (0==index->number_of_indices)
						{
							delete *index_address;
							*index_address = 0;
						}
					}
				}
			}
			else
			{
				//display_message(ERROR_MESSAGE, "REMOVE_OBJECTS_FROM_INDEX_THAT.  Invalid argument(s)");
			}
			return (count);
		}

		/** Adds the <add_index> (and the <add_node>) to the parent of the <node>. */
		static int ADD_INDEX_TO_NODE_PARENT(object_type *add_index,
			INDEX_NODE *add_node, INDEX_NODE *node)
		{
			int return_code = 0;
			INDEX_NODE *parent = node->parent;
			if (parent)
			{
				INDEX_NODE **child, **new_child;
				object_type **index,**new_index;
				/* find place in index node */
				index=parent->indices;
				int i = parent->number_of_indices;
				identifier_type add_identifier = add_index->get_identifier();
				_Compare compare;
				while ((i > 0) && compare((*index)->get_identifier(), add_identifier))
				{
					i--;
					index++;
				}
				if (2*B_TREE_ORDER>parent->number_of_indices)
				{
					/* add object to node (node does not need splitting) */
					add_node->parent=parent;
					index=(parent->indices)+(parent->number_of_indices);
					child=(parent->children)+((parent->number_of_indices)+1);
					while (i>0)
					{
						index--;
						child--;
						*(index+1)= *index;
						*(child+1)= *child;
						i--;
					}
					*index=add_index;
					*child=add_node;
					(parent->number_of_indices)++;
					return_code=1;
				}
				else
				{
					/* node needs splitting */
					/* create a new node */
					INDEX_NODE *new_parent = createStem();
					if (new_parent)
					{
						object_type *split_index = 0;
						if (i == 0)
						{
							// see optimisation comment below
							split_index = (parent->indices)[2*B_TREE_ORDER - 1];
						}
						else if (i > B_TREE_ORDER)
						{
							split_index = (parent->indices)[B_TREE_ORDER - 1];
						}
						else if (i < B_TREE_ORDER)
						{
							split_index = (parent->indices)[B_TREE_ORDER];
						}
						else
						{
							split_index = add_index;
						}
						if (ADD_INDEX_TO_NODE_PARENT(split_index, new_parent, parent))
						{
							if (i == 0)
							{
								// optimisation for adding to end of btree otherwise only 50% stem occupancy
								// for typical case of adding from lowest to highest identifier
								node->parent = new_parent;
								add_node->parent = new_parent;
								new_parent->children[0] = node;
								new_parent->children[1] = add_node;
								new_parent->indices[0] = add_index;
								new_parent->number_of_indices = 1;
								--parent->number_of_indices;
							}
							else if (i<B_TREE_ORDER)
							{
								index=(parent->indices)+(2*B_TREE_ORDER);
								child=(parent->children)+(2*B_TREE_ORDER);
								new_index=(new_parent->indices)+B_TREE_ORDER;
								new_child=(new_parent->children)+B_TREE_ORDER;
								int j = B_TREE_ORDER - i - 1;
								while (i>0)
								{
									index--;
									new_index--;
									*new_index= *index;
									*new_child= *child;
									(*child)->parent=new_parent;
									child--;
									new_child--;
									i--;
								}
								new_index--;
								*new_index=add_index;
								*new_child=add_node;
								add_node->parent=new_parent;
								while (j>0)
								{
									index--;
									new_index--;
									*new_index= *index;
									new_child--;
									*new_child= *child;
									(*child)->parent=new_parent;
									child--;
									j--;
								}
								new_child--;
								*new_child= *child;
								(*child)->parent=new_parent;
								new_parent->number_of_indices=B_TREE_ORDER;
								parent->number_of_indices=B_TREE_ORDER;
							}
							else
							{
								index=(parent->indices)+B_TREE_ORDER;
								child=(parent->children)+B_TREE_ORDER;
								new_index=new_parent->indices;
								new_child=new_parent->children;
								for (int j = B_TREE_ORDER; j > 0; j--)
								{
									*new_index= *index;
									index++;
									new_index++;
									child++;
									new_child++;
									*new_child= *child;
									(*child)->parent=new_parent;
								}
								i -= B_TREE_ORDER;
								if (0==i)
								{
									(parent->children)[B_TREE_ORDER]=node;
									(new_parent->children)[0]=add_node;
									add_node->parent=new_parent;
								}
								else
								{
									i--;
									index=(parent->indices)+(B_TREE_ORDER-1);
									child=(parent->children)+B_TREE_ORDER;
									(new_parent->children)[0]= *child;
									(*child)->parent=new_parent;
									while (i>0)
									{
										index--;
										child--;
										*(index+1)= *index;
										*(child+1)= *child;
										i--;
									}
									*index=add_index;
									*child=add_node;
									add_node->parent=parent;
								}
								new_parent->number_of_indices=B_TREE_ORDER;
								parent->number_of_indices=B_TREE_ORDER;
							}
							return_code=1;
						}
					}
					else
					{
						// display_message(ERROR_MESSAGE,"ADD_INDEX_TO_NODE_PARENT.  Could not create new index node");
					}
				}
			}
			else
			{
				/* create new root node */
				if (0 != (parent = createStem()))
				{
					parent->number_of_indices=1;
					(parent->indices)[0]=add_index;
					(parent->children)[0]=node;
					node->parent=parent;
					(parent->children)[1]=add_node;
					add_node->parent=parent;
					return_code=1;
				}
				else
				{
					// display_message(ERROR_MESSAGE,"ADD_INDEX_TO_NODE_PARENT.  Could not create new root node");
				}
			}
			return (return_code);
		}

		/** Adds an <object> to the <index>. */
		static int ADD_OBJECT_TO_INDEX(object_type *object, INDEX_NODE **index)
		{
			int return_code = 0;
			/* find the leaf node that should contain the object */
			INDEX_NODE *leaf_node = (*index)->FIND_LEAF_NODE_IN_INDEX(object->get_identifier());
			/* check that the object is not already in the leaf node */
			object_type **leaf_index = leaf_node->indices + (leaf_node->number_of_indices - 1);
			int i = leaf_node->number_of_indices;
			_Compare compare;
			identifier_type add_identifier = object->get_identifier();
			while ((i > 0) && !compare((*leaf_index)->get_identifier(), add_identifier))
			{
				--i;
				--leaf_index;
			}
			i = leaf_node->number_of_indices - i;
			if ((i > 0) && !compare(add_identifier, leaf_index[1]->get_identifier()))
			{
				// display_message(ERROR_MESSAGE, "ADD_OBJECT_TO_INDEX.  Object already in index");
			}
			else
			{
				if (2*B_TREE_ORDER > leaf_node->number_of_indices)
				{
					/* add object to leaf node (leaf node does not need splitting) */
					leaf_index=(leaf_node->indices)+(leaf_node->number_of_indices);
					while (i>0)
					{
						leaf_index--;
						*(leaf_index+1)= *leaf_index;
						i--;
					}
					*leaf_index = object->access();
					(leaf_node->number_of_indices)++;
					return_code=1;
				}
				else
				{
					/* leaf node needs splitting */
					INDEX_NODE *new_node = createLeaf();
					if (new_node)
					{
						object_type *split_index;
						if (i == 0)
						{
							// see optimisation comment below
							split_index = (leaf_node->indices)[2*B_TREE_ORDER - 1];
						}
						else
						{
							split_index = (leaf_node->indices)[B_TREE_ORDER - 1];
						}
						if (ADD_INDEX_TO_NODE_PARENT(split_index,new_node,leaf_node))
						{
							return_code=1;
							if ((*index)->parent)
							{
								/* update root node */
								*index=(*index)->parent;
							}
							if (i == 0)
							{
								// optimisation for adding to end of btree otherwise only 50% leaf occupancy
								// for typical case of adding from lowest to highest identifier
								new_node->indices[0] = object->access();
								new_node->number_of_indices = 1;
							}
							else if (i<=B_TREE_ORDER)
							{
								leaf_index=(leaf_node->indices)+(2*B_TREE_ORDER);
								object_type **new_index=(new_node->indices)+B_TREE_ORDER;
								int j=B_TREE_ORDER-i;
								while (i>0)
								{
									leaf_index--;
									*new_index= *leaf_index;
									new_index--;
									i--;
								}
								*new_index = object->access();
								while (j>0)
								{
									leaf_index--;
									new_index--;
									*new_index= *leaf_index;
									j--;
								}
								new_node->number_of_indices=B_TREE_ORDER+1;
								leaf_node->number_of_indices=B_TREE_ORDER;
							}
							else
							{
								leaf_index=(leaf_node->indices)+B_TREE_ORDER;
								object_type **new_index=new_node->indices;
								for (int j=B_TREE_ORDER;j>0;j--)
								{
									*new_index= *leaf_index;
									leaf_index++;
									new_index++;
								}
								new_node->number_of_indices=B_TREE_ORDER;
								i -= B_TREE_ORDER;
								leaf_index=(leaf_node->indices)+B_TREE_ORDER;
								while (i>0)
								{
									leaf_index--;
									*(leaf_index+1)= *leaf_index;
									i--;
								}
								*leaf_index = object->access();
								leaf_node->number_of_indices=B_TREE_ORDER+1;
							}
						}
					}
					else
					{
						// display_message(ERROR_MESSAGE, "ADD_OBJECT_TO_INDEX.  Could not create new leaf node");
					}
				}
			}
			return (return_code);
		}

		/** Returns true if the <object> is in the INDEX_NODE. */
		inline int IS_OBJECT_IN_INDEX(object_type *object)
		{
			int return_code = 0;
			INDEX_NODE *leaf_node = FIND_LEAF_NODE_IN_INDEX(object->get_identifier());
			if (leaf_node)
			{
				object_type **leaf_index = leaf_node->indices;
				int i = leaf_node->number_of_indices;
				_Compare compare;
				identifier_type identifier = object->get_identifier();
				while ((i > 0) && compare((*leaf_index)->get_identifier(), identifier))
				{
					i--;
					leaf_index++;
				}
				if ((i>0)&&(object== *leaf_index))
				{
					return_code = 1;
				}
			}
			return (return_code);
		}

		/** find the object with the specified identifier */
		inline object_type *FIND_BY_IDENTIFIER_IN_INDEX(identifier_type identifier)
		{
			object_type *object = 0;
			INDEX_NODE *leaf_node = FIND_LEAF_NODE_IN_INDEX(identifier);
			if (leaf_node)
			{
				object_type **index = leaf_node->indices;
				int i = leaf_node->number_of_indices;
				_Compare compare;
				while ((i > 0) && compare((*index)->get_identifier(), identifier))
				{
					i--;
					index++;
				}
				if ((i > 0) && !compare(identifier, (*index)->get_identifier()))
				{
					object= *index;
				}
			}
			return (object);
		}

		/** If <conditional> is not NULL, the "first" object in the <index> that
		 * <conditional> returns true is returned, otherwise the "first" object in the
		 * <index> is returned.
		 */
		object_type *FIRST_OBJECT_IN_INDEX_THAT(
			CONDITIONAL_FUNCTION *conditional, void *user_data)
		{
			object_type *object = 0;
			if (children)
			{
				INDEX_NODE **child = children;
				int i = number_of_indices;
				while ((i >= 0) && !object)
				{
					object = (*child)->FIRST_OBJECT_IN_INDEX_THAT(conditional,user_data);
					child++;
					i--;
				}
			}
			else
			{
				if (conditional)
				{
					int i = number_of_indices;
					object_type **object_index = indices;
					while ((i > 0) && !((conditional)(*object_index, user_data)))
					{
						i--;
						object_index++;
					}
					if (i>0)
					{
						object= *object_index;
					}
				}
				else
				{
					object = indices[0];
				}
			}
			return (object);
		}

		/** Calls <iterator> for each object in the <index>. */
		int FOR_EACH_OBJECT_IN_INDEX(ITERATOR_FUNCTION *iterator, void *user_data)
		{
			int return_code = 1;
			int i = number_of_indices;
			if (children)
			{
				INDEX_NODE **child = children;
				while ((i >= 0) && (0 != (return_code =
					(*child)->FOR_EACH_OBJECT_IN_INDEX(iterator,user_data))))
				{
					child++;
					i--;
				}
			}
			else
			{
				object_type **object_index = indices;
				while ((i > 0) && (0 != (return_code = (iterator)(*object_index, user_data))))
				{
					i--;
					object_index++;
				}
			}
			return (return_code);
		}

		// recursive function to get statistics about efficiency of btree storage
		void get_statistics(int current_depth, int& stem_count, int& leaf_count,
			int& min_leaf_depth, int& max_leaf_depth, double& cumulative_leaf_depth,
			double& cumulative_stem_occupancy, double& cumulative_leaf_occupancy) const
		{
			if (children)
			{
				++stem_count;
				cumulative_stem_occupancy += (double)number_of_indices / (double)(2*B_TREE_ORDER + 1);
				for (int i = 0; i <= number_of_indices; ++i)
				{
					children[i]->get_statistics(current_depth + 1, stem_count, leaf_count,
						min_leaf_depth, max_leaf_depth, cumulative_leaf_depth,
						cumulative_stem_occupancy, cumulative_leaf_occupancy);
				}
			}
			else
			{
				++leaf_count;
				cumulative_leaf_depth += (double)current_depth;
				cumulative_leaf_occupancy += (double)number_of_indices / (double)(2*B_TREE_ORDER);
				if ((current_depth < min_leaf_depth) || (0 == min_leaf_depth))
				{
					min_leaf_depth = current_depth;
				}
				else if (current_depth > max_leaf_depth)
				{
					max_leaf_depth = current_depth;
				}
			}
		}

		struct iterator
		{
			INDEX_NODE *node;
			int index;
			int parent_index;

			iterator(INDEX_NODE *index) :
				node(index),
				index(0),
				parent_index(0)
			{
				while (node && node->children)
				{
					node = node->children[0];
				}
			}

			bool operator==(const iterator& other)
			{
				return (node == other.node) && (index == other.index);
			}

			object_type* operator*()
			{
				if (node)
					return node->indices[index];
				return 0;
			}

			iterator& operator++()
			{
				if (node)
				{
					++index;
					if (index >= node->number_of_indices)
					{
						if (node->parent)
						{
							index = 0;
							++parent_index;
							if (parent_index <= node->parent->number_of_indices)
							{
								node = node->parent->children[parent_index];
							}
							else
							{
								node = node->parent;
								INDEX_NODE *parent = node->parent;
								while (parent)
								{
									for (parent_index = parent->number_of_indices; 0 <= parent_index; --parent_index)
									{
										if (parent->children[parent_index] == node)
										{
											++parent_index;
											break;
										}
									}
									if (parent_index <= parent->number_of_indices)
									{
										node = parent->children[parent_index];
										break;
									}
									else
									{
										node = parent;
										parent = node->parent;
									}
								}
								if (!parent)
								{
									set_to_end();
								}
							}
							if (node)
							{
								while (node->children)
								{
									parent_index = 0;
									node = node->children[0];
								}
							}
						}
						else
						{
							set_to_end();
						}
					}
				}
				return *this;
			}

			inline bool not_at_end()
			{
				return (0 != node);
			}

			inline void set_to_end()
			{
				// set to 'end'
				node = 0;
				index = 2*B_TREE_ORDER;
			}

		};

	}; // INDEX_NODE

	typedef typename INDEX_NODE::iterator int_iterator;

public:

	/**
	 * A specialised iterator class which wraps a reference to a container and an
	 * iterator in it, suitable for use from external API because the container
	 * cannot be destroyed before the iterator.
	 */
	struct ext_iterator
	{
		cmzn_btree *container; // non-accessed pointer to owning btree, cleared if container destroyed
		int_iterator iter;
		ext_iterator *next_iterator; // for linked list of active_iterators in btree to invalidate on change

		ext_iterator(cmzn_btree *container) :
			container(container),
			iter(container->index),
			next_iterator(0)
		{
			container->addIterator(this);
		}

		~ext_iterator()
		{
			if (container)
				container->removeIterator(this);
		}

		object_type *next()
		{
			if (iter.not_at_end())
			{
				object_type *object = *iter;
				++iter;
				return object->access();
			}
			return 0;
		}

		object_type *next_non_access()
		{
			if (iter.not_at_end())
			{
				object_type *object = *iter;
				++iter;
				return object;
			}
			return 0;
		}

		void invalidate()
		{
			iter.set_to_end();
			if (container)
			{
				container->removeIterator(this);
				container = 0;
			}
		}

	};

private:

	INDEX_NODE *index;
	int count;
	int access_count;
	mutable cmzn_btree *next, *prev; // linked list of related sets
	object_type *temp_removed_object; // removed while changing identifier
	ext_iterator *active_iterators; // linked-list of iterators to invalidate if btree is modified or destroyed

	cmzn_btree() :
		index(0),
		count(0),
		access_count(1),
		next(0),
		prev(0),
		temp_removed_object(0),
		active_iterators(0)
	{
		next = this;
		prev = this;
	}

	/** copy constructor */
	cmzn_btree(const cmzn_btree& source) :
		index(source.index->duplicate()),
		count(source.count),
		access_count(1),
		next(source.next),
		prev(&source),
		temp_removed_object(0),
		active_iterators(0)
	{
		source.next = this;
		next->prev = this;
	}

	/** creates a related set, not a copy constructor */
	cmzn_btree(const cmzn_btree *source) :
		index(0),
		count(0),
		access_count(1),
		next(source->next),
		prev(const_cast<cmzn_btree *>(source)),
		temp_removed_object(0),
		active_iterators(0)
	{
		source->next = this;
		next->prev = this;
	}

	~cmzn_btree()
	{
		this->invalidateIterators();
		delete index;
		index = 0;
		prev->next = next;
		next->prev = prev;
	}

	void addIterator(ext_iterator *iter)
	{
		iter->next_iterator = active_iterators;
		active_iterators = iter;
	}

	void removeIterator(ext_iterator *iter)
	{
		ext_iterator *tmp = active_iterators;
		ext_iterator **prev_address = &active_iterators;
		while (tmp)
		{
			if (tmp == iter)
			{
				*prev_address = tmp->next_iterator;
				tmp->next_iterator = 0;
				break;
			}
			prev_address = &(tmp->next_iterator);
			tmp = tmp->next_iterator;
		}
	}

	/** call when btree is modified */
	inline void invalidateIterators()
	{
		while (active_iterators)
		{
			active_iterators->invalidate();
		}
	}

public:

	static cmzn_btree *create_independent()
	{
		return new cmzn_btree();
	}

	cmzn_btree *create_related() const
	{
		return new cmzn_btree(this);
	}

	cmzn_btree *create_copy() const
	{
		return new cmzn_btree(*this);
	}

	cmzn_btree& operator=(const cmzn_btree& source)
	{
		if (&source == this)
			return *this;
		invalidateIterators();
		const cmzn_btree *related_set = this->next;
		while (related_set != this)
		{
			if (related_set == &source)
			{
				break;
			}
			related_set = related_set->next;
		}
		delete index;
		if (source.index)
		{
			index = source.index->duplicate();
			count = source.count;
		}
		else
		{
			index = 0;
			count = 0;
		}
		if (related_set == this)
		{
			// copy from unrelated set: switch linked-lists
			this->next->prev = this->prev;
			this->prev->next = this->next;
			this->prev = const_cast<cmzn_btree *>(&source);
			this->next = source.next;
			source.next->prev = this;
			source.next = this;
		}
		return *this;
	}

	inline cmzn_btree *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_btree **set_address)
	{
		if (set_address && *set_address)
		{
			if (0 >= (--(*set_address)->access_count))
			{
				delete *set_address;
			}
			*set_address = 0;
			return 1;
		}
		return 0;
	}

	inline int erase(object_type *object)
	{
		int return_code = INDEX_NODE::REMOVE_OBJECT_FROM_INDEX(object, &index);
		if (return_code)
		{
			--count;
			invalidateIterators();
		}
		return return_code;
	}

	inline int erase_conditional(CONDITIONAL_FUNCTION *conditional, void *user_data)
	{
		int number_removed = INDEX_NODE::REMOVE_OBJECTS_FROM_INDEX_THAT(conditional, user_data, &index);
		if (number_removed > 0)
		{
			count -= number_removed;
			invalidateIterators();
		}
		return 1;
	}

	inline void clear()
	{
		invalidateIterators();
		delete index;
		index = 0;
		count = 0;
	}

	inline int insert(object_type *object)
	{
		int return_code = 0;
		if (!index)
		{
			index = INDEX_NODE::createLeaf();
		}
		if (index)
		{
			return_code = INDEX_NODE::ADD_OBJECT_TO_INDEX(object, &index);
			if (return_code)
			{
				++count;
				invalidateIterators();
			}
		}
		return return_code;
	}

	inline int size() const
	{
		return count;
	}

	inline int contains(object_type *object) const
	{
		if (index)
			return index->IS_OBJECT_IN_INDEX(object);
		return 0;
	}

	inline object_type *find_first_object_that(
		CONDITIONAL_FUNCTION *conditional, void *user_data) const
	{
		if (index)
			return index->FIRST_OBJECT_IN_INDEX_THAT(conditional, user_data);
		return 0;
	}

	inline int for_each_object(
		ITERATOR_FUNCTION *iterator, void *user_data) const
	{
		if (index)
			return index->FOR_EACH_OBJECT_IN_INDEX(iterator, user_data);
		return 1;
	}

	inline object_type *find_object_by_identifier(identifier_type identifier)
	{
		if (index)
			return index->FIND_BY_IDENTIFIER_IN_INDEX(identifier);
		return 0;
	}

	bool begin_identifier_change(object_type *object)
	{
		cmzn_btree *related_btree = this;
		do
		{
			if (related_btree->contains(object))
			{
				related_btree->temp_removed_object = object->access();
				related_btree->erase(object);
			}
			else
			{
				related_btree->temp_removed_object = 0;
			}
			related_btree = related_btree->next;
		}
		while (related_btree != this);
		return true;
	}

	void end_identifier_change()
	{
		cmzn_btree *related_btree = this;
		do
		{
			if (related_btree->temp_removed_object)
			{
				related_btree->insert(related_btree->temp_removed_object); // check success?
				related_btree->temp_removed_object->deaccess(&related_btree->temp_removed_object);
			}
			related_btree = related_btree->next;
		}
		while (related_btree != this);
	}

	void get_statistics(int& stem_count, int& leaf_count,
		int& min_leaf_depth, int& max_leaf_depth, double& mean_leaf_depth,
		double& mean_stem_occupancy, double& mean_leaf_occupancy) const
	{
		stem_count = 0;
		leaf_count = 0;
		min_leaf_depth = 0;
		max_leaf_depth = 0;
		mean_leaf_depth = 0.0;
		mean_stem_occupancy = 0.0;
		mean_leaf_occupancy = 0.0;
		if (index)
		{
			index->get_statistics(/*current_depth*/1, stem_count, leaf_count,
				min_leaf_depth, max_leaf_depth, mean_leaf_depth, mean_stem_occupancy, mean_leaf_occupancy);
			if (stem_count > 0)
			{
				mean_stem_occupancy /= (double)stem_count;
			}
			if (leaf_count > 0)
			{
				mean_leaf_depth /= (double)leaf_count;
				mean_leaf_occupancy /= (double)leaf_count;
			}
		}
	}
};

#endif /* !defined (CMZN_BTREE_HPP) */
