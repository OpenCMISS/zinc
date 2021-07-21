/**
 * @file cmiss_btree_index.hpp
 *
 * N-way binary tree container template class.
 * Variant specialised for storing indexes in an owner object's array-like
 * data structure, e.g. a DsLabels.
 * The owner object must provide a getIdentifier(index) methods.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_BTREE_INDEX_HPP)
#define CMZN_BTREE_INDEX_HPP

template<class owner_type, typename object_type, typename identifier_type,
	int invalid_object = -1, int btreeOrder = 10> class cmzn_btree_index
{
private:

	class BTreeNode
	{
	private:
		int number_of_indices;
		// break point indices (identifiers accessed through indices)
		// all actual objects are stored in leaf nodes only, elsewhere they are sorting indices
		object_type indices[2*btreeOrder];
		BTreeNode *parent;
		BTreeNode **children;

		BTreeNode(bool leaf) :
			number_of_indices(0),
			parent(0),
			children(leaf ? 0 : new BTreeNode*[2*btreeOrder + 1])
		{
		}

		/** deep copy constructor */
		BTreeNode(const BTreeNode &source) :
			number_of_indices(source.number_of_indices),
			parent(0),
			children(source.children ? new BTreeNode*[2*btreeOrder + 1] : 0)
		{
			bool leaf = !children;
			const object_type *source_index = source.indices;
			object_type *duplicate_index = this->indices;
			if (leaf)
			{
				for (int i = number_of_indices; i > 0; --i)
				{
					*duplicate_index = *source_index;
					++source_index;
					++duplicate_index;
				}
			}
			else
			{
				BTreeNode **source_child = source.children;
				BTreeNode **duplicate_child = children;
				for (int i = number_of_indices; i >= 0; --i)
				{
					*duplicate_child = new BTreeNode(**source_child);
					(*duplicate_child)->parent = this;
					++source_child;
					++duplicate_child;
					if (i)
					{
						*duplicate_index = *source_index;
						++source_index;
						++duplicate_index;
					}
				}
			}
		}

	public:

		~BTreeNode()
		{
			if (children)
			{
				for (int i = 0; i <= number_of_indices; ++i)
				{
					delete children[i];
				}
				delete[] children;
			}
		}

		static inline BTreeNode *createLeaf()
		{
			return new BTreeNode(true);
		}

		static inline BTreeNode *createStem()
		{
			return new BTreeNode(false);
		}

		BTreeNode *duplicate() const
		{
			return new BTreeNode(*this);
		}

		/** Finds the leaf node that should contain the object with the specified identifier */
		const BTreeNode *findLeafNode(const owner_type& owner, identifier_type identifier) const
		{
			if (children)
			{
				BTreeNode **child = this->children + this->number_of_indices;
				int i = number_of_indices;
				const object_type *object_index = this->indices + (this->number_of_indices - 1);
				while ((i > 0) && !(owner.getIdentifier(*object_index) < identifier))
				{
					--object_index;
					--child;
					--i;
				}
				return (*child)->findLeafNode(owner, identifier);
			}
			return this;
		}

		/** Removes an <object> from the <index>. */
		static int removeObject(owner_type& owner, object_type object, BTreeNode **index_address)
		{
			int return_code = 0;
			if (index_address)
			{
				BTreeNode *child, *index;
				if (0 != (index= *index_address))
				{
					/* find the object within the node */
					int i = 0;
					identifier_type identifier = owner.getIdentifier(object);
					while ((i < index->number_of_indices) && (owner.getIdentifier(index->indices[i]) < identifier))
					{
						i++;
					}
					if (index->children)
					{
						if (0 != (return_code = removeObject(owner, object,
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
				//display_message(ERROR_MESSAGE, "BTreeNode::removeObject.  Invalid argument(s)");
			}
			return (return_code);
		}

		/** Removes each object that pred returns true for. */
		template <class UnaryPredicate>
		static int removeObjectsConditional(BTreeNode **index_address,
			UnaryPredicate& pred)
		{
			int count = 0;
			/* check the arguments */
			if (index_address)
			{
				int i, j, original_number_of_indices;
				BTreeNode *child, *index;
				if (0 != (index= *index_address))
				{
					if (index->children)
					{
						original_number_of_indices = index->number_of_indices;
						for (i = 0; i <= original_number_of_indices; i++)
						{
							count += removeObjectsConditional(&(index->children[i]), pred);
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
								if (pred(index->indices[i]))
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
						/* loop over all indices */
						j=0;
						for (i=0;i<index->number_of_indices;i++)
						{
							index->indices[j]=index->indices[i];
							if (pred(index->indices[j]))
							{
								/* delete the object */
								count++;
							}
							else
							{
								j++;
							}
						}
						index->number_of_indices=j;
						/* have we killed the node? */
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
				//display_message(ERROR_MESSAGE, "BTreeNode::removeObjectsConditional.  Invalid argument(s)");
			}
			return (count);
		}

		/** Adds the <add_index> (and the <add_node>) to the parent of the <node>. */
		static int addToParent(owner_type &owner, object_type add_index,
			BTreeNode *add_node, BTreeNode *node)
		{
			int return_code = 0;
			BTreeNode *parent = node->parent;
			if (parent)
			{
				BTreeNode **child, **new_child;
				object_type *index,*new_index;
				/* find place in index node */
				index=parent->indices;
				int i = parent->number_of_indices;
				identifier_type add_identifier = owner.getIdentifier(add_index);
				while ((i > 0) && (owner.getIdentifier(*index) < add_identifier))
				{
					i--;
					index++;
				}
				if (2*btreeOrder>parent->number_of_indices)
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
					BTreeNode *new_parent = createStem();
					if (new_parent)
					{
						object_type split_index = add_index;
						if (i == 0)
						{
							// see optimisation comment below
							split_index = (parent->indices)[2*btreeOrder - 1];
						}
						else if (i > btreeOrder)
						{
							split_index = (parent->indices)[btreeOrder - 1];
						}
						else if (i < btreeOrder)
						{
							split_index = (parent->indices)[btreeOrder];
						}
						if (addToParent(owner, split_index, new_parent, parent))
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
							else if (i<btreeOrder)
							{
								index=(parent->indices)+(2*btreeOrder);
								child=(parent->children)+(2*btreeOrder);
								new_index=(new_parent->indices)+btreeOrder;
								new_child=(new_parent->children)+btreeOrder;
								int j = btreeOrder - i - 1;
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
								new_parent->number_of_indices=btreeOrder;
								parent->number_of_indices=btreeOrder;
							}
							else
							{
								index=(parent->indices)+btreeOrder;
								child=(parent->children)+btreeOrder;
								new_index=new_parent->indices;
								new_child=new_parent->children;
								for (int j = btreeOrder; j > 0; j--)
								{
									*new_index= *index;
									index++;
									new_index++;
									child++;
									new_child++;
									*new_child= *child;
									(*child)->parent=new_parent;
								}
								i -= btreeOrder;
								if (0==i)
								{
									(parent->children)[btreeOrder]=node;
									(new_parent->children)[0]=add_node;
									add_node->parent=new_parent;
								}
								else
								{
									i--;
									index=(parent->indices)+(btreeOrder-1);
									child=(parent->children)+btreeOrder;
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
								new_parent->number_of_indices=btreeOrder;
								parent->number_of_indices=btreeOrder;
							}
							return_code=1;
						}
					}
					else
					{
						// display_message(ERROR_MESSAGE,"BTreeNode::addToParent.  Could not create new index node");
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
					// display_message(ERROR_MESSAGE,"BTreeNode::addToParent.  Could not create new root node");
				}
			}
			return (return_code);
		}

		/** Adds an <object> to the <index>. */
		static int addObject(owner_type &owner, object_type object, BTreeNode **index)
		{
			int return_code = 0;
			/* find the leaf node that should contain the object */
			identifier_type add_identifier = owner.getIdentifier(object);
			BTreeNode *leaf_node = const_cast<BTreeNode*>((*index)->findLeafNode(owner, add_identifier));
			/* check that the object is not already in the leaf node */
			object_type *leaf_index = leaf_node->indices + (leaf_node->number_of_indices - 1);
			int i = leaf_node->number_of_indices;
			while ((i > 0) && !(owner.getIdentifier(*leaf_index) < add_identifier))
			{
				--i;
				--leaf_index;
			}
			i = leaf_node->number_of_indices - i;
			if ((i > 0) && !(add_identifier < owner.getIdentifier(leaf_index[1])))
			{
				// display_message(ERROR_MESSAGE, "BTreeNode::addObject.  Object already in index");
			}
			else
			{
				if (2*btreeOrder > leaf_node->number_of_indices)
				{
					/* add object to leaf node (leaf node does not need splitting) */
					leaf_index=(leaf_node->indices)+(leaf_node->number_of_indices);
					while (i>0)
					{
						leaf_index--;
						*(leaf_index+1)= *leaf_index;
						i--;
					}
					*leaf_index = object;
					(leaf_node->number_of_indices)++;
					return_code=1;
				}
				else
				{
					/* leaf node needs splitting */
					BTreeNode *new_node = createLeaf();
					if (new_node)
					{
						object_type split_index;
						if (i == 0)
						{
							// see optimisation comment below
							split_index = (leaf_node->indices)[2*btreeOrder - 1];
						}
						else
						{
							split_index = (leaf_node->indices)[btreeOrder - 1];
						}
						if (addToParent(owner,split_index,new_node,leaf_node))
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
								new_node->indices[0] = object;
								new_node->number_of_indices = 1;
							}
							else if (i<=btreeOrder)
							{
								leaf_index=(leaf_node->indices)+(2*btreeOrder);
								object_type *new_index=(new_node->indices)+btreeOrder;
								int j=btreeOrder-i;
								while (i>0)
								{
									leaf_index--;
									*new_index= *leaf_index;
									new_index--;
									i--;
								}
								*new_index = object;
								while (j>0)
								{
									leaf_index--;
									new_index--;
									*new_index= *leaf_index;
									j--;
								}
								new_node->number_of_indices=btreeOrder+1;
								leaf_node->number_of_indices=btreeOrder;
							}
							else
							{
								leaf_index=(leaf_node->indices)+btreeOrder;
								object_type *new_index=new_node->indices;
								for (int j=btreeOrder;j>0;j--)
								{
									*new_index= *leaf_index;
									leaf_index++;
									new_index++;
								}
								new_node->number_of_indices=btreeOrder;
								i -= btreeOrder;
								leaf_index=(leaf_node->indices)+btreeOrder;
								while (i>0)
								{
									leaf_index--;
									*(leaf_index+1)= *leaf_index;
									i--;
								}
								*leaf_index = object;
								leaf_node->number_of_indices=btreeOrder+1;
							}
						}
					}
					else
					{
						// display_message(ERROR_MESSAGE, "BTreeNode::addObject.  Could not create new leaf node");
					}
				}
			}
			return (return_code);
		}

		/** Returns true if the <object> is in the BTreeNode. */
		inline bool containsObject(owner_type &owner, object_type object)
		{
			identifier_type identifier = owner.getIdentifier(object);
			const BTreeNode *leaf_node = findLeafNode(owner, identifier);
			if (leaf_node)
			{
				const object_type *leaf_index = leaf_node->indices;
				int i = leaf_node->number_of_indices;
				while ((i > 0) && (owner.getIdentifier(*leaf_index) < identifier))
				{
					i--;
					leaf_index++;
				}
				if ((i>0)&&(object== *leaf_index))
				{
					return true;
				}
			}
			return false;
		}

		/** find the object with the specified identifier */
		inline object_type findObjectByIdentifier(const owner_type &owner, identifier_type identifier) const
		{
			const BTreeNode *leaf_node = findLeafNode(owner, identifier);
			if (leaf_node)
			{
				const object_type *leaf_index = leaf_node->indices;
				int i = leaf_node->number_of_indices;
				while ((i > 0) && (owner.getIdentifier(*leaf_index) < identifier))
				{
					i--;
					leaf_index++;
				}
				if ((i > 0) && !(identifier < owner.getIdentifier(*leaf_index)))
				{
					return *leaf_index;
				}
			}
			return invalid_object;
		}

		object_type getFirstObject()
		{
			if (this->children)
				return this->children[0]->getFirstObject();
			return this->indices[0];
		}

		object_type getLastObject()
		{
			if (this->children)
				return this->children[this->number_of_indices]->getLastObject();
			return this->indices[this->number_of_indices - 1];
		}

		/**
		 * @return the first object in the <index> for which pred is true
		 */
		template <class UnaryPredicate>
		object_type findFirstObjectConditional(UnaryPredicate& pred)
		{
			if (children)
			{
				object_type object;
				BTreeNode **child = children;
				for (int i = this->number_of_indices; i >= 0; --i)
				{
					object = (*child)->findFirstObjectConditional(pred);
					if (object != invalid_object)
						return object;
					++child;
				}
			}
			else
			{
				object_type *object_index = indices;
				for (int i = this->number_of_indices; i > 0; --i)
				{
					if (pred(*object_index))
						return *object_index;
					++object_index;
				}
			}
			return invalid_object;
		}

		/**
		 * Calls <iterator> for each object in this index, stopping if it returns false
		 * @return  Boolean true on success, false on error
		 */
		template <class UnaryPredicate>
		bool forEachObject(UnaryPredicate &iterator)
		{
			if (children)
			{
				BTreeNode **child = children;
				for (int i = this->number_of_indices; i >= 0; --i)
				{
					if (!(*child)->forEachObject(iterator))
						return false;
					++child;
				}
			}
			else
			{
				object_type *object_index = indices;
				for (int i = this->number_of_indices; i > 0; --i)
				{
					if (!pred(*object_index))
						return false;
					++object_index;
				}
			}
			return true;
		}

		// recursive function to get statistics about efficiency of btree storage
		void get_statistics(int current_depth, int& stem_count, int& leaf_count,
			int& min_leaf_depth, int& max_leaf_depth, double& cumulative_leaf_depth,
			double& cumulative_stem_occupancy, double& cumulative_leaf_occupancy) const
		{
			if (children)
			{
				++stem_count;
				cumulative_stem_occupancy += (double)number_of_indices / (double)(2*btreeOrder + 1);
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
				cumulative_leaf_occupancy += (double)number_of_indices / (double)(2*btreeOrder);
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
			BTreeNode *node;
			int index;
			int parent_index;

			iterator(BTreeNode *index) :
				node(index),
				index(-1), // before start
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

			object_type operator*()
			{
				if (node && (index >= 0))
					return node->indices[index];
				return invalid_object;
			}

			void operator++()
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
								BTreeNode *parent = node->parent;
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
									// set to restart
									set_to_start(this->node);
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
							// set to restart
							set_to_start(this->node);
						}
					}
				}
			}

			inline bool not_at_end()
			{
				return (0 != node);
			}

			inline void set_to_end()
			{
				// set to 'end'
				this->node = 0;
				index = -1; // ready to start again
			}

			inline void set_to_start(BTreeNode *root_node)
			{
				this->node = root_node;
				this->index = -1;
				this->parent_index = 0;
				while (node && node->children)
				{
					node = node->children[0];
				}
			}
			
			bool set_object(const owner_type &owner, BTreeNode *root_node, object_type object)
			{
				if (object == invalid_object)
				{
					this->set_to_start(root_node);
					return true;
				}
				if (root_node)
				{
					this->node = const_cast<BTreeNode *>(root_node->findLeafNode(owner, owner.getIdentifier(object)));
					if (this->node)
					{
						object_type *leaf_index = this->node->indices;
						int i = this->node->number_of_indices;
						while ((i > 0) && (*leaf_index != object))
						{
							++leaf_index;
							--i;
						}
						if (i)
						{
							this->index = this->node->number_of_indices - i;
							if (this->node->parent)
							{
								BTreeNode **child = this->node->parent->children;
								i = this->node->parent->number_of_indices;
								while (i >= 0)
								{
									if (*child == this->node)
									{
										this->parent_index = this->node->parent->number_of_indices - i;
										return true;
									}
									++child;
									--i;
								}
							}
							else
							{
								this->parent_index = 0;
								return true;
							}
						}
					}
				}
				this->set_to_end();
				return false;
			}

		};

	}; // BTreeNode

	typedef typename BTreeNode::iterator int_iterator;

public:

	/**
	 * A specialised iterator class which wraps a reference to a container and an
	 * iterator in it, suitable for use from external API. The container safely
	 * invalidates all iterators when it is destroyed.
	 */
	struct ext_iterator
	{
		const cmzn_btree_index *container; // non-accessed pointer to owning btree, cleared if container destroyed
		int_iterator iter;
		ext_iterator *next_iterator; // for linked list of active_iterators in btree to invalidate on change

		ext_iterator(const cmzn_btree_index *container) :
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

		object_type get_object()
		{
			return *iter;
		}

		bool set_object(const owner_type &owner, object_type object)
		{
			if (container)
				return this->iter.set_object(owner, container->index, object);
			return false;
		}

		// restarts after last invalid_object
		object_type next()
		{
			++iter;
			return *iter;
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

	BTreeNode *index;
	int count;
	int access_count;
	mutable cmzn_btree_index *next, *prev; // linked list of related sets
	object_type temp_removed_object; // removed while changing identifier
	mutable ext_iterator *active_iterators; // linked-list of iterators to invalidate if btree is modified or destroyed

public:

	cmzn_btree_index() :
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
	cmzn_btree_index(const cmzn_btree_index& source) :
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
	cmzn_btree_index(const cmzn_btree_index *source) :
		index(0),
		count(0),
		access_count(1),
		next(source->next),
		prev(const_cast<cmzn_btree_index *>(source)),
		temp_removed_object(0),
		active_iterators(0)
	{
		source->next = this;
		next->prev = this;
	}

	~cmzn_btree_index()
	{
		this->invalidateIterators();
		delete index;
		index = 0;
		prev->next = next;
		next->prev = prev;
	}

	void addIterator(ext_iterator *iter) const
	{
		iter->next_iterator = active_iterators;
		active_iterators = iter;
	}

	void removeIterator(ext_iterator *iter) const
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

	static cmzn_btree_index *create_independent()
	{
		return new cmzn_btree_index();
	}

	cmzn_btree_index *create_related() const
	{
		return new cmzn_btree_index(this);
	}

	cmzn_btree_index *create_copy() const
	{
		return new cmzn_btree_index(*this);
	}

	cmzn_btree_index& operator=(const cmzn_btree_index& source)
	{
		if (&source == this)
			return *this;
		invalidateIterators();
		const cmzn_btree_index *related_set = this->next;
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
			this->prev = const_cast<cmzn_btree_index *>(&source);
			this->next = source.next;
			source.next->prev = this;
			source.next = this;
		}
		return *this;
	}

	inline cmzn_btree_index *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_btree_index **set_address)
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

	inline bool erase(owner_type &owner, object_type object)
	{
		if (BTreeNode::removeObject(owner, object, &index))
		{
			--count;
			invalidateIterators();
			return true;
		}
		return false;
	}

	template <class UnaryPredicate>
	inline bool erase_conditional(UnaryPredicate& pred)
	{
		int number_removed = BTreeNode::removeObjectsConditional(&index, pred);
		if (number_removed > 0)
		{
			count -= number_removed;
			invalidateIterators();
		}
		return true;
	}

	inline void clear()
	{
		invalidateIterators();
		delete index;
		index = 0;
		count = 0;
	}

	/** Note: caller must ensure object is valid! */
	inline bool insert(owner_type &owner, object_type object)
	{
		if (!index)
		{
			index = BTreeNode::createLeaf();
		}
		if (index)
		{
			if (BTreeNode::addObject(owner, object, &index))
			{
				++count;
				invalidateIterators();
				return true;
			}
		}
		return false;
	}

	inline int size() const
	{
		return count;
	}

	inline bool contains(owner_type &owner, object_type object) const
	{
		if (index)
			return index->containsObject(owner, object);
		return false;
	}

	template <class UnaryPredicate>
	inline object_type find_first_object_that(UnaryPredicate& pred) const
	{
		if (index)
			return index->findFirstObjectConditional(pred);
		return invalid_object;
	}

	template <class UnaryPredicate>
	inline bool for_each_object(UnaryPredicate& iterator) const
	{
		if (index)
			return index->forEachObject(iterator);
		return true;
	}

	inline object_type find_object_by_identifier(const owner_type &owner, identifier_type identifier) const
	{
		if (index)
			return index->findObjectByIdentifier(owner, identifier);
		return invalid_object;
	}

	inline object_type get_first_object() const
	{
		if (index)
			return index->getFirstObject();
		return invalid_object;
	}

	inline object_type get_last_object() const
	{
		if (index)
			return index->getLastObject();
		return invalid_object;
	}

	bool begin_identifier_change(owner_type &owner, object_type object)
	{
		cmzn_btree_index *related_btree = this;
		do
		{
			if (related_btree->contains(owner, object))
			{
				related_btree->temp_removed_object = object;
				related_btree->erase(owner, object);
			}
			else
			{
				related_btree->temp_removed_object = invalid_object;
			}
			related_btree = related_btree->next;
		}
		while (related_btree != this);
		return true;
	}

	bool end_identifier_change(owner_type &owner)
	{
		bool result = true;
		cmzn_btree_index *related_btree = this;
		do
		{
			if (related_btree->temp_removed_object != invalid_object)
			{
				if (!related_btree->insert(owner, related_btree->temp_removed_object))
					result = false;
				related_btree->temp_removed_object = invalid_object;
			}
			related_btree = related_btree->next;
		}
		while (related_btree != this);
		return result;
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
