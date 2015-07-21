#ifndef DARTS_H_
#define DARTS_H_

// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#define DARTS_VERSION "0.32"
#define DARTS_CLONE_VERSION "0.32f"

#ifdef _MSC_VER
#include <stdio.h>
#include <share.h>
#endif  // _MSC_VER

#include <algorithm>
#include <cstdio>
#include <exception>
#include <stack>
#include <vector>

// Defines macros for debugging.
#ifdef _DEBUG
#include <iostream>
#define LOG (std::cerr << "DEBUG:" << __LINE__ << ": ")
#define LOG_VALUE(value) (LOG << # value << " = " << value << '\n')
#else  // _DEBUG
#define LOG
#define LOG_VALUE(value)
#endif  // _DEBUG

// Defines macros for throwing exceptions with line numbers.
#define THROW(msg) THROW_RELAY(__LINE__, msg)
#define THROW_RELAY(line, msg) THROW_FINAL(line, msg)
#define THROW_FINAL(line, msg) throw \
	DoubleArrayException("darts-clone-" DARTS_CLONE_VERSION \
		" [" # line "]: " msg)

namespace Darts
{

// This class provides basic types used in this library.
class DoubleArrayBasicTypes
{
public:
	typedef char char_type;
	typedef unsigned char uchar_type;

	// Must be a 32-bit unsigned integer type.
	typedef unsigned int base_type;
	typedef std::size_t size_type;

	// Must be a 32-bit signed integer type.
	typedef int value_type;

private:
	// No objects.
	DoubleArrayBasicTypes();
};

// Exception class.
class DoubleArrayException : public std::exception
{
private:
	const char *msg_;

public:
	// A constant string should be passed.
	template <int Size>
	explicit DoubleArrayException(const char (&msg)[Size]) : msg_(msg) {}

	const char *what() const throw() { return msg_; }
};

// File I/O.
class DoubleArrayFile
{
public:
	typedef DoubleArrayBasicTypes::size_type size_type;

private:
	std::FILE *file_;

	// Copies are not allowed.
	DoubleArrayFile(const DoubleArrayFile &);
	DoubleArrayFile &operator=(const DoubleArrayFile &);

public:
	DoubleArrayFile(const char *file_name, const char *mode) : file_(0)
	{
		if (!file_name)
			THROW("null file name");
		else if (!mode)
			THROW("null file mode");

#ifdef _MSC_VER
		// To avoid warnings against std::fopen().
		file_ = _fsopen(file_name, mode, _SH_DENYWR);
#else
		file_ = std::fopen(file_name, mode);
#endif
	}
	~DoubleArrayFile() { is_open() && std::fclose(file_); }

	// Moves a file pointer.
	bool seek(size_type offset, int whence = SEEK_SET)
	{ return std::fseek(file_, static_cast<long>(offset), whence) == 0; }

	// Gets the file size.
	bool size(size_type &file_size)
	{
		long current_position = ftell(file_);
		if (current_position == -1L || !seek(0, SEEK_END))
			return false;

		long pos_end = ftell(file_);
		if (pos_end == -1L || !seek(current_position, SEEK_SET))
			return false;

		file_size = static_cast<size_type>(pos_end);
		return true;
	}

	// Reads units from a file.
	template <typename T>
	bool read(T *buf, size_type nmemb)
	{ return std::fread(buf, sizeof(T), nmemb, file_) == nmemb; }
	// Writes units to a file.
	template <typename T>
	bool write(const T *buf, size_type nmemb)
	{ return std::fwrite(buf, sizeof(T), nmemb, file_) == nmemb; }

	// Checks if a file is opened or not.
	bool is_open() const { return file_ ? true : false; }
};

// A range for keys.
class DoubleArrayKeyRange
{
public:
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;

public:
	DoubleArrayKeyRange(size_type begin, size_type end, size_type depth)
		: begin_(begin), end_(end), depth_(depth), index_(0) {}

	void set_index(base_type index) { index_ = index; }

	size_type begin() const { return begin_; }
	size_type end() const { return end_; }
	size_type depth() const { return depth_; }
	base_type index() const { return index_; }

private:
	size_type begin_;
	size_type end_;
	size_type depth_;
	base_type index_;
};

// An object pool for building a dawg.
template <typename ValueType>
class DoubleArrayPool
{
public:
	typedef ValueType value_type;

	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;

	// Number of nodes in each block.
	enum { BLOCK_SIZE = 1024 };

private:
	std::vector<value_type *> blocks_;
	base_type size_;

	// Copies are not allowed.
	DoubleArrayPool(const DoubleArrayPool &);
	DoubleArrayPool &operator=(const DoubleArrayPool &);

public:
	DoubleArrayPool() : blocks_(), size_(0) {}
	~DoubleArrayPool() { clear(); }

	// Deletes all blocks.
	void clear()
	{
		for (size_type i = 0; i < blocks_.size(); ++i)
			delete [] blocks_[i];

		size_ = 0;
		std::vector<value_type *>(0).swap(blocks_);
	}

	// Gets a new object.
	base_type get()
	{
		if (size_ == static_cast<base_type>(BLOCK_SIZE * blocks_.size()))
			blocks_.push_back(new value_type[BLOCK_SIZE]);
		return size_++;
	}

	// Reads the number of values.
	base_type size() const { return size_; }

	// Accesses a value.
	value_type &operator[](base_type index)
	{ return blocks_[index / BLOCK_SIZE][index % BLOCK_SIZE]; }
	// Accesses a value.
	const value_type &operator[](base_type index) const
	{ return blocks_[index / BLOCK_SIZE][index % BLOCK_SIZE]; }
};

// Directed Acyclic Word Graph (DAWG).
class DoubleArrayDawg
{
public:
	typedef DoubleArrayBasicTypes::char_type char_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;
	typedef DoubleArrayBasicTypes::value_type value_type;

	typedef std::pair<base_type, base_type> pair_type;
	typedef DoubleArrayPool<pair_type> pair_pool_type;
	typedef DoubleArrayPool<char_type> char_pool_type;

	enum { DEFAULT_HASH_TABLE_SIZE = 1 << 8 };

private:
	pair_pool_type state_pool_;
	char_pool_type label_pool_;
	std::vector<base_type> hash_table_;
	std::vector<base_type> unfixed_states_;
	std::vector<base_type> unused_states_;
	base_type num_of_merged_states_;

	// Copies are not allowed.
	DoubleArrayDawg(const DoubleArrayDawg &);
	DoubleArrayDawg &operator=(const DoubleArrayDawg &);

public:
	DoubleArrayDawg() : state_pool_(), label_pool_(),
		hash_table_(DEFAULT_HASH_TABLE_SIZE, 0),
		unfixed_states_(), unused_states_(), num_of_merged_states_(0) {}

	// Builds a dawg.
	void build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths, const value_type *values,
		int (*progress_func)(size_type, size_type))
	{
		// For the root state and the root node.
		get();
		set_label(0, 0);
		unfixed_states_.push_back(0);

		// Inserts and merges keys.
		size_type key_id = num_of_keys;
		size_type max_progress = num_of_keys + num_of_keys / 4;
		while (key_id--)
		{
			const char_type *key = keys[key_id];
			size_type length = lengths ? lengths[key_id] : length_of(key);
			value_type value = values[key_id];

			insert_key(key, length, value);

			if (progress_func)
				progress_func(num_of_keys - key_id, max_progress);
		}

		// Merges states corresponding to the first key.
		merge(0);

		LOG_VALUE(state_pool_.size());
		LOG_VALUE(hash_table_.size());
		LOG_VALUE(unfixed_states_.size());
		LOG_VALUE(unused_states_.size());
		LOG_VALUE(num_of_merged_states_);

		std::vector<base_type>(0).swap(hash_table_);
	}

	// Gets the size of a state pool.
	base_type size() const
	{ return static_cast<base_type>(state_pool_.size()); }
	// Gets the number of states.
	base_type num_of_states() const
	{ return static_cast<base_type>(size() - unused_states_.size()); }

public:
	// Clears a state.
	void clear_state(base_type index)
	{ state_pool_[index] = pair_type(0, 0); }

	// Sets an index of a next state.
	void set_child(base_type index, base_type child)
	{ state_pool_[index].first = child << 1; }
	// Sets an index of a sibling state.
	void set_sibling(base_type index, base_type sibling)
	{ state_pool_[index].second = sibling << 1; }
	// Sets a value of a leaf state.
	void set_value(base_type index, value_type value)
	{ state_pool_[index].first = (value << 1) | 1; }

	// Sets a label.
	void set_label(base_type index, char_type label)
	{ label_pool_[index] = label; }

	// Reads an index of a next state.
	base_type child(base_type index) const
	{ return get_value(state_pool_[index].first); }
	// Reads an index of a sibling state.
	base_type sibling(base_type index) const
	{ return get_value(state_pool_[index].second); }
	// Checks if a state is a leaf or not.
	bool is_leaf(base_type index) const
	{ return get_bit(state_pool_[index].first); }
	// Reads a value of a leaf state.
	value_type value(base_type index) const
	{ return get_value(state_pool_[index].first); }

	// Reads a label.
	char_type label(base_type index) const
	{ return label_pool_[index]; }

private:
	// Inserts a key.
	void insert_key(const char_type *key, size_type length, value_type value)
	{
		base_type index = 0;
		base_type key_pos = 0;

		// Finds a separate state.
		for ( ; key_pos <= length; ++key_pos)
		{
			base_type child_index = child(index);
			if (!child_index)
				break;
			else if (label(child_index) != get_key_label(key, length, key_pos))
			{
				merge(index);
				break;
			}
			index = child_index;
		}

		// Adds new states.
		for ( ; key_pos <= length; ++key_pos)
		{
			base_type child_index = get();
			set_sibling(child_index, child(index));
			set_label(child_index, get_key_label(key, length, key_pos));
			unfixed_states_.push_back(child_index);
			set_child(index, child_index);
			index = child_index;
		}
		set_value(index, value);
	}

	// Merges common states recursively.
	void merge(base_type index)
	{
		while (unfixed_states_.back() != index)
		{
			base_type unfixed_index = unfixed_states_.back();

			if (size() >= hash_table_.size() - (hash_table_.size() >> 2))
				expand_hash_table();

			size_type hash_id;
			base_type matched_index = find_state(unfixed_index, &hash_id);
			if (matched_index)
			{
				unget(unfixed_index);
				unfixed_index = matched_index;
				++num_of_merged_states_;
			}
			else
				hash_table_[hash_id] = unfixed_index;

			unfixed_states_.resize(unfixed_states_.size() - 1);
			set_child(unfixed_states_.back(), unfixed_index);
		}
	}

private:
	// Finds a state from a hash table.
	base_type find_state(base_type index, size_type *hash_id) const
	{
		const pair_type &state = state_pool_[index];
		char_type label = label_pool_[index];

		*hash_id = hash(state.first, state.second, label) % hash_table_.size();
		for ( ; ; *hash_id = (*hash_id + 1) % hash_table_.size())
		{
			base_type state_id = hash_table_[*hash_id];
			if (!state_id)
				break;

			if (state == state_pool_[state_id]
				&& label == label_pool_[state_id])
				return state_id;
		}

		return 0;
	}

	// Expands a hash table.
	void expand_hash_table()
	{
		std::vector<base_type> free_states(unfixed_states_);
		free_states.insert(free_states.end(),
			unused_states_.begin(), unused_states_.end());
		std::sort(free_states.begin(), free_states.end());

		size_type hash_table_size = hash_table_.size() << 1;
		std::vector<base_type>(0).swap(hash_table_);
		hash_table_.resize(hash_table_size, 0);

		// Inserts states into a new hash table.
		base_type state_id = 0;
		for (size_type i = 0; i < free_states.size(); ++i, ++state_id)
		{
			for ( ; state_id < free_states[i]; ++state_id)
			{
				size_type hash_id;
				find_state(state_id, &hash_id);
				hash_table_[hash_id] = state_id;
			}
		}
		for ( ; state_id < size(); ++state_id)
		{
			size_type hash_id;
			find_state(state_id, &hash_id);
			hash_table_[hash_id] = state_id;
		}
	}

	// Hash function.
	static base_type hash(base_type a, base_type b, base_type c)
	{
		a = a - b; a = a - c; a = a ^ rotate_to_right(c, 13);
		b = b - c; b = b - a; b = b ^ (a << 8);
		c = c - a; c = c - b; c = c ^ rotate_to_right(b, 13);
		a = a - b; a = a - c; a = a ^ rotate_to_right(c, 12);
		b = b - c; b = b - a; b = b ^ (a << 16);
		c = c - a; c = c - b; c = c ^ rotate_to_right(b, 5);
		a = a - b; a = a - c; a = a ^ rotate_to_right(c, 3);
		b = b - c; b = b - a; b = b ^ (a << 10);
		c = c - a; c = c - b; c = c ^ rotate_to_right(b, 15);

		return c;
	}

	// Rotate shift for an unsigned integer.
	static base_type rotate_to_right(base_type value, int shift)
	{ return (value >> shift) | (value << (sizeof(value) - shift)); }

private:
	// Gets a new object.
	base_type get()
	{
		base_type index = 0;
		if (unused_states_.empty())
		{
			index = state_pool_.get();
			label_pool_.get();
		}
		else
		{
			index = unused_states_.back();
			unused_states_.resize(unused_states_.size() - 1);
		}
		clear_state(index);
		return index;
	}
	// Ungets an object.
	void unget(base_type index) { unused_states_.push_back(index); }

	// Gets a least significant bit.
	static bool get_bit(base_type value) { return (value & 1) ? true : false; }
	// Gets a value except for a least significant bit.
	static base_type get_value(base_type value) { return value >> 1; }
	// Gets a byte from a key.
	static char_type get_key_label(const char_type *key,
		size_type length, size_type key_pos)
	{ return (key_pos < length) ? key[key_pos] : 0; }

	// Scans a given key and returns its length.
	static size_type length_of(const char_type *key)
	{
		size_type count = 0;
		while (key[count])
			++count;
		return count;
	}
};

// A unit of a double-array.
class DoubleArrayUnit
{
public:
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::value_type value_type;

	static const base_type OFFSET_MAX = static_cast<base_type>(1) << 21;
	static const base_type IS_LEAF_BIT = static_cast<base_type>(1) << 31;
	static const base_type HAS_LEAF_BIT = static_cast<base_type>(1) << 8;
	static const base_type EXTENSION_BIT = static_cast<base_type>(1) << 9;

private:
	base_type base_;

public:
	DoubleArrayUnit() : base_(0) {}

	// Sets a flag to show that a unit has a leaf as a child.
	void set_has_leaf() { base_ |= HAS_LEAF_BIT; }
	// Sets a value to a leaf unit.
	void set_value(value_type value)
	{ base_ = static_cast<base_type>(value) | IS_LEAF_BIT; }
	// Sets a label to a non-leaf unit.
	void set_label(uchar_type label)
	{ base_ = (base_ & ~static_cast<base_type>(0xFF)) | label; }
	// Sets an offset to a non-leaf unit.
	void set_offset(base_type offset)
	{
		if (offset >= (OFFSET_MAX << 8))
			THROW("too large offset");

		base_ &= IS_LEAF_BIT | HAS_LEAF_BIT | 0xFF;
		if (offset < OFFSET_MAX)
			base_ |= (offset << 10);
		else
			base_ |= (offset << 2) | EXTENSION_BIT;
	}

	// Checks if a unit has a leaf as a child or not.
	bool has_leaf() const { return (base_ & HAS_LEAF_BIT) ? true : false; }
	// Checks if a unit corresponds to a leaf or not.
	value_type value() const
	{ return static_cast<value_type>(base_ & ~IS_LEAF_BIT); }
	// Reads a label with a leaf flag from a non-leaf unit.
	base_type label() const { return base_ & (IS_LEAF_BIT | 0xFF); }
	// Reads an offset to child units from a non-leaf unit.
	base_type offset() const
	{ return (base_ >> 10) << ((base_ & EXTENSION_BIT) >> 6); }
	// Reads an offset to child units from a non-leaf unit by using if.
	base_type offset_if() const
	{ return (base_ & EXTENSION_BIT) ? ((base_ >> 10) << 8) : (base_ >> 10); }
};

// An extra information which are used for building a double-array.
class DoubleArrayExtra
{
public:
	typedef DoubleArrayBasicTypes::base_type base_type;

private:
	base_type lo_values_;
	base_type hi_values_;

public:
	DoubleArrayExtra() : lo_values_(0), hi_values_(0) {}

	void clear() { lo_values_ = hi_values_ = 0; }

	// Sets if a unit is fixed or not.
	void set_is_fixed() { lo_values_ |= 1; }
	// Sets an index of the next unused unit.
	void set_next(base_type next)
	{ lo_values_ = (lo_values_ & 1) | (next << 1); }
	// Sets if an index is used as an offset or not.
	void set_is_used() { hi_values_ |= 1; }
	// Sets an index of the previous unused unit.
	void set_prev(base_type prev)
	{ hi_values_ = (hi_values_ & 1) | (prev << 1); }

	// Reads if a unit is fixed or not.
	bool is_fixed() const { return (lo_values_ & 1) == 1; }
	// Reads an index of the next unused unit.
	base_type next() const { return lo_values_ >> 1; }
	// Reads if an index is used as an offset or not.
	bool is_used() const { return (hi_values_ & 1) == 1; }
	// Reads an index of the previous unused unit.
	base_type prev() const { return hi_values_ >> 1; }
};

// A class for building a double-array.
class DoubleArrayBuilder
{
public:
	typedef DoubleArrayBasicTypes::char_type char_type;
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;
	typedef DoubleArrayBasicTypes::value_type value_type;

	typedef DoubleArrayKeyRange range_type;
	typedef DoubleArrayDawg dawg_type;
	typedef DoubleArrayUnit unit_type;
	typedef DoubleArrayExtra extra_type;

	enum
	{
		BLOCK_SIZE = 256,
		NUM_OF_UNFIXED_BLOCKS = 16,
		UNFIXED_SIZE = BLOCK_SIZE * NUM_OF_UNFIXED_BLOCKS,
	};

private:
	// Masks for offsets.
	static const base_type LOWER_MASK = unit_type::OFFSET_MAX - 1;
	static const base_type UPPER_MASK = ~LOWER_MASK;

private:
	size_type num_of_keys_;
	const char_type * const *keys_;
	const size_type *lengths_;
	const value_type *values_;

	int (*progress_func_)(size_type, size_type);
	size_type progress_;
	size_type max_progress_;

	std::vector<unit_type> units_;
	std::vector<extra_type *> extras_;
	std::vector<uchar_type> labels_;
	base_type unfixed_index_;
	size_type num_of_unused_units_;

	// Copies are not allowed.
	DoubleArrayBuilder(const DoubleArrayBuilder &);
	DoubleArrayBuilder &operator=(const DoubleArrayBuilder &);

public:
	DoubleArrayBuilder() : num_of_keys_(0), keys_(0), lengths_(0), values_(0),
		progress_func_(0), progress_(0), max_progress_(0), units_(), extras_(),
		labels_(), unfixed_index_(0), num_of_unused_units_(0) {}
	~DoubleArrayBuilder()
	{
		for (size_type i = 0; i < extras_.size(); ++i)
			delete [] extras_[i];
	}

	// Copies parameters and builds a double-array.
	bool build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths, const value_type *values,
		int (*progress_func)(size_type, size_type))
	{
		// Copies parameters.
		num_of_keys_ = num_of_keys;
		keys_ = keys;
		lengths_ = lengths;
		values_ = values;
		progress_func_ = progress_func;

		test_keys();
		LOG_VALUE(num_of_keys_);

		// Builds a naive trie.
		if (!values_)
			build_trie();
		else
			build_dawg();
		LOG_VALUE(num_of_units());

		// Shrinks a double-array.
		std::vector<unit_type>(units_).swap(units_);

		return true;
	}

	// Returns a reference to a built double-array.
	std::vector<unit_type> &units_buf() { return units_; }

private:
	// Builds a trie.
	void build_trie()
	{
		// 0 is reserved for the root.
		reserve_unit(0);

		// To avoid invalid transitions.
		extras(0).set_is_used();
		units(0).set_offset(1);
		units(0).set_label(0);

		progress_ = 0;
		max_progress_ = num_of_keys_;

		if (num_of_keys_)
			build_double_array();

		// Adjusts all unfixed blocks.
		fix_all_blocks();
	}

	// Builds a double-array.
	void build_double_array()
	{
		std::stack<range_type> range_stack;
		range_stack.push(range_type(0, num_of_keys_, 0));

		std::vector<range_type> child_ranges;
		while (!range_stack.empty())
		{
			range_type range = range_stack.top();
			range_stack.pop();

			// Lists labels to child units.
			size_type child_begin = range.begin();
			labels_.push_back(keys(child_begin, range.depth()));
			for (size_type i = range.begin() + 1; i != range.end(); ++i)
			{
				if (!labels_.back())
					progress();

				if (keys(i, range.depth()) != labels_.back())
				{
					labels_.push_back(keys(i, range.depth()));
					child_ranges.push_back(
						range_type(child_begin, i, range.depth() + 1));
					child_begin = i;
				}
			}
			if (!labels_.back())
				progress();
			child_ranges.push_back(
				range_type(child_begin, range.end(), range.depth() + 1));

			// Finds a good offset.
			base_type offset = find_offset(range.index());
			units(range.index()).set_offset(range.index() ^ offset);

			for (size_type i = child_ranges.size(); i > 0; )
			{
				// Reserves a child unit.
				base_type child = offset ^ labels_[--i];
				reserve_unit(child);

				if (!labels_[i])
				{
					// Sets a value for a leaf unit.
					units(range.index()).set_has_leaf();
					units(child).set_value(values_ ? values_[range.begin() + i]
						: static_cast<value_type>(range.begin() + i));
				}
				else
				{
					units(child).set_label(labels_[i]);
					child_ranges[i].set_index(child);
					range_stack.push(child_ranges[i]);
				}
			}
			extras(offset).set_is_used();

			labels_.clear();
			child_ranges.clear();
		}
	}

private:
	// Builds a dawg.
	void build_dawg()
	{
		dawg_type dawg;
		dawg.build(num_of_keys_, keys_, lengths_, values_, progress_func_);

		std::vector<base_type> offset_values(dawg.size(), 0);

		// 0 is reserved for the root.
		reserve_unit(0);

		// To avoid invalid transitions.
		extras(0).set_is_used();
		units(0).set_offset(1);
		units(0).set_label(0);

		progress_ = dawg.num_of_states() * 4;
		max_progress_ = dawg.num_of_states() * 5;

		build_double_array(dawg, offset_values, 0, 0);

		// Fixes remaining blocks.
		fix_all_blocks();
	}

	// Builds a double-array.
	void build_double_array(const dawg_type &dawg,
		std::vector<base_type> &offset_values,
		base_type dawg_index, base_type da_index)
	{
		progress();

		if (dawg.is_leaf(dawg_index))
			return;

		// Already arranged.
		base_type dawg_child_index = dawg.child(dawg_index);
		if (offset_values[dawg_child_index])
		{
			base_type offset = offset_values[dawg_child_index] ^ da_index;
			if (!(offset & LOWER_MASK) || !(offset & UPPER_MASK))
			{
				if (dawg.label(dawg_child_index) == '\0')
					units(da_index).set_has_leaf();
				units(da_index).set_offset(offset);
				return;
			}
		}

		// Finds a good offset.
		base_type offset = arrange_child_nodes(dawg, dawg_index, da_index);
		offset_values[dawg_child_index] = offset;

		// Builds a double-array in depth-first order.
		do
		{
			base_type da_child_index =
				offset ^ static_cast<uchar_type>(dawg.label(dawg_child_index));
			build_double_array(dawg, offset_values,
				dawg_child_index, da_child_index);
			dawg_child_index = dawg.sibling(dawg_child_index);
		} while (dawg_child_index);
	}

	// Arranges child nodes.
	base_type arrange_child_nodes(const dawg_type &dawg,
		base_type dawg_index, base_type da_index)
	{
		labels_.clear();

		base_type dawg_child_index = dawg.child(dawg_index);
		while (dawg_child_index)
		{
			labels_.push_back(
				static_cast<uchar_type>(dawg.label(dawg_child_index)));
			dawg_child_index = dawg.sibling(dawg_child_index);
		}

		// Finds a good offset.
		base_type offset = find_offset(da_index);
		units(da_index).set_offset(da_index ^ offset);

		dawg_child_index = dawg.child(dawg_index);
		for (size_type i = 0; i < labels_.size(); ++i)
		{
			base_type da_child_index = offset ^ labels_[i];
			reserve_unit(da_child_index);

			if (dawg.is_leaf(dawg_child_index))
			{
				units(da_index).set_has_leaf();
				units(da_child_index).set_value(dawg.value(dawg_child_index));
			}
			else
				units(da_child_index).set_label(labels_[i]);

			dawg_child_index = dawg.sibling(dawg_child_index);
		}
		extras(offset).set_is_used();

		return offset;
	}

private:
	// Finds a good offset.
	base_type find_offset(base_type index) const
	{
		if (unfixed_index_ >= num_of_units())
			return num_of_units() | (index & 0xFF);

		// Scans unused units to find a good offset.
		base_type unfixed_index = unfixed_index_;
		do
		{
			base_type offset = unfixed_index ^ labels_[0];
			if (is_good_offset(index, offset))
				return offset;
			unfixed_index = extras(unfixed_index).next();
		} while (unfixed_index != unfixed_index_);

		return num_of_units() | (index & 0xFF);
	}

	// Checks if a given offset is valid or not.
	bool is_good_offset(base_type index, base_type offset) const
	{
		static const base_type LOWER_MASK = unit_type::OFFSET_MAX - 1;
		static const base_type UPPER_MASK = ~LOWER_MASK;

		if (extras(offset).is_used())
			return false;

		base_type relative_offset = index ^ offset;
		if ((relative_offset & LOWER_MASK) && (relative_offset & UPPER_MASK))
			return false;

		// Finds a collision.
		for (size_type i = 1; i < labels_.size(); ++i)
		{
			if (extras(offset ^ labels_[i]).is_fixed())
				return false;
		}

		return true;
	}

	// Reserves an unused unit.
	void reserve_unit(base_type index)
	{
		if (index >= num_of_units())
			expand_double_array();

		// Removes an unused unit from a circular linked list.
		if (index == unfixed_index_)
		{
			unfixed_index_ = extras(index).next();
			if (unfixed_index_ == index)
				unfixed_index_ = num_of_units();
		}
		extras(extras(index).prev()).set_next(extras(index).next());
		extras(extras(index).next()).set_prev(extras(index).prev());
		extras(index).set_is_fixed();
	}

	// Expands a double-array.
	void expand_double_array()
	{
		base_type src_num_of_units = num_of_units();
		base_type src_num_of_blocks = num_of_blocks();

		base_type dest_num_of_units = src_num_of_units + BLOCK_SIZE;
		base_type dest_num_of_blocks = src_num_of_blocks + 1;

		// Fixes an old block.
		if (dest_num_of_blocks > NUM_OF_UNFIXED_BLOCKS)
			fix_block(src_num_of_blocks - NUM_OF_UNFIXED_BLOCKS);

		units_.resize(dest_num_of_units);
		extras_.resize(dest_num_of_blocks, 0);

		// Allocates memory to a new block.
		if (dest_num_of_blocks > NUM_OF_UNFIXED_BLOCKS)
		{
			base_type block_id = src_num_of_blocks - NUM_OF_UNFIXED_BLOCKS;
			std::swap(extras_[block_id], extras_.back());
			for (base_type i = src_num_of_units; i < dest_num_of_units; ++i)
				extras(i).clear();
		}
		else
			extras_.back() = new extra_type[BLOCK_SIZE];

		// Creates a circular linked list for a new block.
		for (base_type i = src_num_of_units + 1; i < dest_num_of_units; ++i)
		{
			extras(i - 1).set_next(i);
			extras(i).set_prev(i - 1);
		}

		extras(src_num_of_units).set_prev(dest_num_of_units - 1);
		extras(dest_num_of_units - 1).set_next(src_num_of_units);

		// Merges 2 circular linked lists.
		extras(src_num_of_units).set_prev(extras(unfixed_index_).prev());
		extras(dest_num_of_units - 1).set_next(unfixed_index_);

		extras(extras(unfixed_index_).prev()).set_next(src_num_of_units);
		extras(unfixed_index_).set_prev(dest_num_of_units - 1);
	}

	// Fixes all blocks to avoid invalid transitions.
	void fix_all_blocks()
	{
		base_type begin = 0;
		if (num_of_blocks() > NUM_OF_UNFIXED_BLOCKS)
			begin = num_of_blocks() - NUM_OF_UNFIXED_BLOCKS;
		base_type end = num_of_blocks();

		for (base_type block_id = begin; block_id != end; ++block_id)
			fix_block(block_id);
	}

	// Adjusts labels of unused units in a given block.
	void fix_block(base_type block_id)
	{
		base_type begin = block_id * BLOCK_SIZE;
		base_type end = begin + BLOCK_SIZE;

		// Finds an unused offset.
		base_type unused_offset_for_label = 0;
		for (base_type offset = begin; offset != end; ++offset)
		{
			if (!extras(offset).is_used())
			{
				unused_offset_for_label = offset;
				break;
			}
		}

		// Labels of unused units are modified.
		for (base_type index = begin; index != end; ++index)
		{
			if (!extras(index).is_fixed())
			{
				reserve_unit(index);
				units(index).set_label(static_cast<uchar_type>(
					index ^ unused_offset_for_label));
				++num_of_unused_units_;
			}
		}
	}

private:
	// Tests a given set of keys.
	void test_keys() const
	{
		for (size_type i = 0; i < num_of_keys_; ++i)
		{
			// Finds a null pointer.
			if (!keys_[i])
				THROW("null pointer in key set");

			// Finds a null character.
			if (lengths_)
			{
				if (!lengths_)
					THROW("zero length key");

				for (size_type j = 0; j < lengths_[i]; ++j)
				{
					if (!keys_[i][j])
						THROW("null character in key");
				}
			} else if (!keys_[i][0])
				THROW("zero length key");

			// Finds a negative value.
			if (values_ && values_[i] < 0)
				THROW("negative value");

			// Finds a wrong order.
			if (i > 0)
			{
				int comparison_result = lengths_ ? compare_keys(
					keys_[i - 1], lengths_[i - 1], keys_[i], lengths_[i])
					: compare_keys(keys_[i - 1], keys_[i]);
				if (comparison_result > 0)
					THROW("invalid key order");
			}
		}
	}

	// Compares a pair of keys and returns a result like strcmp().
	static int compare_keys(const char_type *lhs, const char_type *rhs)
	{
		while (*lhs != 0 && *lhs == *rhs)
			++lhs, ++rhs;

		return static_cast<uchar_type>(*lhs) - static_cast<uchar_type>(*rhs);
	}
	// Compares a pair of keys and returns a result like strcmp().
	static int compare_keys(const char_type *lhs_key, size_type lhs_length,
		const char_type *rhs_key, size_type rhs_length)
	{
		size_type min_length = std::min(lhs_length, rhs_length);
		size_type key_pos = 0;

		while (key_pos < min_length && lhs_key[key_pos] == rhs_key[key_pos])
			++key_pos;

		if (key_pos == min_length)
			return static_cast<int>(lhs_length - rhs_length);
		return static_cast<uchar_type>(lhs_key[key_pos])
			- static_cast<uchar_type>(rhs_key[key_pos]);
	}

private:
	// Accesses a unit.
	unit_type &units(base_type index) { return units_[index]; }
	// Accesses a unit.
	const unit_type &units(base_type index) const { return units_[index]; }

	// Accesses an extra information.
	extra_type &extras(base_type index)
	{ return extras_[index / BLOCK_SIZE][index % BLOCK_SIZE]; }
	// Accesses an extra information.
	const extra_type &extras(base_type index) const
	{ return extras_[index / BLOCK_SIZE][index % BLOCK_SIZE]; }

	// Number of units.
	base_type num_of_units() const
	{ return static_cast<base_type>(units_.size()); }
	// Number of blocks.
	base_type num_of_blocks() const
	{ return static_cast<base_type>(extras_.size()); }

private:
	// Notifies a progress.
	void progress()
	{
		if (progress_ >= max_progress_)
			return;

		++progress_;
		if (progress_func_)
			progress_func_(progress_, max_progress_);
	}

	// Reads a byte from a key.
	uchar_type keys(size_type key_id, size_type depth) const
	{
		return (!lengths_ || lengths_[key_id] < depth)
			? keys_[key_id][depth] : 0;
	}
};

// The base class of the DoubleArray.
template <typename ValueType>
class DoubleArrayBase
{
public:
	// For compatibility.
	typedef ValueType result_type;

	typedef DoubleArrayBasicTypes::char_type char_type;
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;
	typedef DoubleArrayBasicTypes::value_type value_type;

	// For compatibility.
	typedef char_type key_type;

	typedef DoubleArrayFile file_type;
	typedef DoubleArrayUnit unit_type;

	struct result_pair_type
	{
		result_type value;
		size_type length;
	};

private:
	const unit_type *units_;
	size_type size_;
	std::vector<unit_type> units_buf_;

	// Copies are not allowed.
	DoubleArrayBase(const DoubleArrayBase &);
	DoubleArrayBase &operator=(const DoubleArrayBase &);

public:
	DoubleArrayBase() : units_(0), size_(0), units_buf_() {}

	// Builds a double-array from a set of keys.
	int build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths = 0, const result_type *values = 0,
		int (*progress_func)(size_type, size_type) = 0)
	{
		DoubleArrayBuilder builder;
		if (!builder.build(num_of_keys, keys, lengths,
			reinterpret_cast<const value_type *>(values), progress_func))
			return -1;
		replace_units_buf(builder.units_buf());
		return 0;
	}

	// Returns the number of units.
	size_type size() const { return size_; }
	// Returns the size of each unit of an array.
	size_type unit_size() const { return sizeof(unit_type); }
	// Always returns the number of units (no use).
	size_type nonzero_size() const { return size(); }
	// Returns the array size.
    size_type total_size() const { return size_ * sizeof(unit_type); }

	// Sets the start address of an array.
	void set_array(const void *ptr, size_type size = 0)
	{
		clear();
		units_ = static_cast<const unit_type *>(ptr);
		size_ = size;

		LOG_VALUE(total_size());
	}
	// Returns the start address of an array.
	const void *array() const { return units_; }

	// Frees allocated memory.
	void clear()
	{
		units_ = 0;
		size_ = 0;
		std::vector<unit_type>(0).swap(units_buf_);
	}

	// Loads a double-array from a file.
	int open(const char *file_name, const char *mode = "rb",
		size_type offset = 0, size_type size = 0)
	{
		file_type file(file_name, mode);
		if (!file.is_open() || !file.seek(offset))
			return -1;

		if (!size)
		{
			if (!file.size(size) || size <= offset)
				return -1;
			size -= offset;
		}
		if ((size % sizeof(unit_type)) != 0)
			return -1;
		size_type num_of_units = size / sizeof(unit_type);

		std::vector<unit_type> new_units_buf(num_of_units);
		if (!file.read(&new_units_buf[0], num_of_units))
			return -1;

		replace_units_buf(new_units_buf);
		return 0;
	}
	// Saves a double-array to a file.
	int save(const char *file_name, const char *mode = "wb",
		size_type offset = 0) const
	{
		file_type file(file_name, mode);
		if (!file.is_open() || !file.seek(offset))
			return -1;

		if (!file.write(units_, size_))
			return -1;
		return 0;
	}

public:
	// Searches a double-array for a given key.
	template <typename ResultType>
	void exactMatchSearch(const char_type *key, ResultType &result,
		size_type length = 0, size_type node_pos = 0) const
	{ result = exactMatchSearch<ResultType>(key, length, node_pos); }

	// Searches a double-array for a given key.
	template <typename ResultType>
	ResultType exactMatchSearch(const char_type *key,
		size_type length = 0, size_type node_pos = 0) const
	{
		if (!length)
			length = length_of(key);

		ResultType result;
		set_result(result, -1, 0);

		// Transitions.
		base_type index = static_cast<base_type>(node_pos);
		unit_type unit = units_[index];
		size_type key_pos = 0;
		while (key_pos < length)
		{
			index ^= unit.offset_if() ^ static_cast<uchar_type>(key[key_pos]);
			unit = units_[index];
			if (unit.label() != static_cast<uchar_type>(key[key_pos]))
				return result;
			++key_pos;
		}

		if (!unit.has_leaf())
			return result;

		unit = units_[index ^ unit.offset()];
		set_result(result, unit.value(), key_pos);
		return result;
	}

	// Searches a double-array for prefixes of a given key.
	template <typename ResultType>
	size_type commonPrefixSearch(const char_type *key,
		ResultType *results, size_type max_num_of_results,
		size_type length = 0, size_type node_pos = 0) const
	{
		if (!length)
			length = length_of(key);

		size_type num_of_results = 0;

		// Transitions.
		base_type index = static_cast<base_type>(node_pos);
		unit_type unit = units_[index];
		for (size_type key_pos = 0; key_pos < length; ++key_pos)
		{
			index ^= unit.offset() ^ static_cast<uchar_type>(key[key_pos]);
			unit = units_[index];
			if (unit.label() != static_cast<uchar_type>(key[key_pos]))
				break;

			if (!unit.has_leaf())
				continue;

			if (num_of_results < max_num_of_results)
			{
				unit_type stray = units_[index ^ unit.offset()];
				set_result(results[num_of_results],
					stray.value(), key_pos + 1);
			}
			++num_of_results;
		}

		return num_of_results;
	}

	// Searches a double-array for a given key.
	value_type traverse(const char_type *key, size_type &node_pos,
		size_type &key_pos, size_type length = 0) const
	{
		if (!length)
			length = length_of(key);

		// Transitions.
		base_type index = static_cast<base_type>(node_pos);
		unit_type unit = units_[index];
		for ( ; key_pos < length; ++key_pos)
		{
			index ^= unit.offset() ^ static_cast<uchar_type>(key[key_pos]);
			unit = units_[index];
			if (unit.label() != static_cast<uchar_type>(key[key_pos]))
				return static_cast<value_type>(-2);

			node_pos = static_cast<size_type>(index);
		}

		if (!unit.has_leaf())
			return static_cast<value_type>(-1);

		unit = units_[index ^ unit.offset()];
		return unit.value();
	}

private:
	// Replaces a current vector with a given vector.
	void replace_units_buf(std::vector<unit_type> &new_units_buf)
	{
		if (new_units_buf.empty())
			THROW("empty unit vector");

		set_array(&new_units_buf[0], new_units_buf.size());
		units_buf_.swap(new_units_buf);
	}

	// Scans a given key and returns its length.
	static size_type length_of(const char_type *key)
	{
		size_type count = 0;
		while (key[count])
			++count;
		return count;
	}

	// Sets a result value.
	static void set_result(result_type &result, value_type value, size_type)
	{ result = static_cast<result_type>(value); }
	// Sets a result pair.
	static void set_result(result_pair_type &result,
		value_type value, size_type length)
	{
		result.value = value;
		result.length = length;
	}
};

typedef DoubleArrayBase<int> DoubleArray;

// For chasen.
template <typename A, typename B, typename ValueType, typename D>
class DoubleArrayImpl : public DoubleArrayBase<ValueType> {};

}  // namespace Darts

// Undefines internal macros.
#undef THROW
#undef THROW_RELAY
#undef THROW_FINAL

#undef LOG
#undef LOG_VALUE

#endif  // DARTS_H_
