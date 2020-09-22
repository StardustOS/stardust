/* Copyright (C) 2018, Ward Jaradat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <os/config.h>

#ifdef ENABLE_PTE

#include <os/tls.h>

tls_pointer
tls_init()
{
	tls_pointer this = (tls_pointer) malloc(sizeof(tls_struct));
	if(this == 0)
	{
		return NULL;
	}
	this->lock = mutex_create();
	return(this);
}

tls_node_pointer
tls_node_init(unsigned key, void *value)
{
	tls_node_pointer this =(tls_node_pointer) malloc(sizeof(tls_node_struct));
	if(this == 0)
	{
		return NULL;
	}
	this->key = key;
	this->value = value;
	this->left = NULL;
	this->right = NULL;
	return this;
}

int
tls_comp(unsigned a, unsigned b)
{
	return a - b;
}

tls_node_pointer *
tls_search(tls_node_pointer *root, unsigned search_key)
{
	tls_node_pointer *this = root;
	int comp_result;
	while(*this != NULL)
	{
		comp_result = tls_comp(search_key, (*this)->key);
		if (comp_result < 0)
		{
			this = &((*this)->left);
		}
		else if (comp_result > 0)
		{
			this = &((*this)->right);
		}
		else
		{
			break;
		}
	}
	return this;
}

int
tls_put(tls_pointer m, unsigned key, void *value)
{
	tls_node_pointer new_node = tls_node_init(key, value);
	if (m == NULL)
	{
		return 0;
	}
	if (new_node == NULL)
	{
		return 0;
	}
	tls_node_pointer *found_node = tls_search(&m->root, new_node->key);
	*found_node = new_node;
	return 1;
}

tls_node_pointer *
tls_get_root(tls_node_pointer *root)
{
	tls_node_pointer *this = root;
	return this;
}

int
tls_alloc(tls_pointer m, unsigned key, void *value)
{
	tls_node_pointer new_node = tls_node_init(key, value);
	if (m == NULL)
	{
		return 0;
	}
	if (new_node == NULL)
	{
		return 0;
	}
	tls_node_pointer *found_node = tls_get_root(&m->root);
	*found_node = new_node;
	return 1;
}

extern void *
tls_get(tls_pointer m, unsigned key)
{
	if (m == NULL)
	{
		return NULL;
	}
	tls_node_pointer *found_node = tls_search(&m->root, key);
	if (*found_node == NULL)
	{
		return NULL;
	}
	return (*found_node)->value;
}

extern int
tls_has_key(tls_pointer m, unsigned key)
{
	if (m == NULL)
	{
		return 0;
	}
	tls_node_pointer *found_node = tls_search(&m->root, key);
	if (*found_node == NULL)
	{
		return 0;
	}
	return 1;
}

extern int
tls_set(tls_pointer m, unsigned key, void *value)
{
	if (m == NULL)
	{
		return 0;
	}
	tls_node_pointer *found_node = tls_search(&m->root, key);
	if (*found_node == NULL)
	{
		return 0;
	}
	(*found_node)->value = value;
	return 1;
}

unsigned
tls_count(tls_node_pointer root)
{
	unsigned count = 0;
	unsigned left = 0;
	unsigned right = 0;
	if (root != NULL)
	{
		count = count + 1;
		if (root->left != NULL)
		{
			left = tls_count(root->left);
		}
		if (root->right != NULL)
		{
			right = tls_count(root->right);
		}
	}
	count = count + left + right;
	return count;
}

#endif
