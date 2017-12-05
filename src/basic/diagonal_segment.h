/****
DIAMOND protein aligner
Copyright (C) 2013-2017 Benjamin Buchfink <buchfink@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#ifndef DIAGONAL_SEGMENT_H_
#define DIAGONAL_SEGMENT_H_

#include "translated_position.h"
#include "score_matrix.h"
#include "sequence.h"

struct Diagonal_segment
{
	Diagonal_segment() :
		len(0)
	{}
	Diagonal_segment(int query_pos, int subject_pos, int len, int score) :
		i(query_pos),
		j(subject_pos),
		len(len),
		score(score)
	{}
	bool empty() const
	{
		return len == 0;
	}
	interval query_range() const
	{
		return interval(i, i + len);
	}
	interval subject_range() const
	{
		return interval(j, j + len);
	}
	int subject_last() const
	{
		return j + len - 1;
	}
	int query_last() const
	{
		return i + len - 1;
	}
	int subject_end() const
	{
		return j + len;
	}
	int query_end() const
	{
		return i + len;
	}
	int diag() const
	{
		return i - j;
	}
	Diagonal_segment intersect(const Diagonal_segment &x) const
	{
		if (diag() != x.diag())
			return Diagonal_segment();
		else {
			const interval q = ::intersect(query_range(), x.query_range());
			return Diagonal_segment(q.begin_, ::intersect(subject_range(), x.subject_range()).begin_, q.length(), 0);
		}
	}
	bool is_enveloped(const Diagonal_segment &x) const
	{
		return score <= x.score
			&& query_range().overlap_factor(x.query_range()) == 1
			&& subject_range().overlap_factor(x.subject_range()) == 1;
	}
	Diagonal_segment transpose() const
	{
		return Diagonal_segment(j, i, len, score);
	}
	int partial_score(int diff) const
	{
		return score*std::max(len - diff, 0) / len;
	}
	bool operator<=(const Diagonal_segment &rhs) const
	{
		return i + len <= rhs.i && j + len <= rhs.j;
	}
	bool operator==(const Diagonal_segment &rhs) const
	{
		return i == rhs.i && j == rhs.j && len == rhs.len;
	}
	static bool cmp_subject(const Diagonal_segment &x, const Diagonal_segment &y)
	{
		return x.j < y.j || (x.j == y.j && x.i < y.i);
	}
	static bool cmp_subject_end(const Diagonal_segment &x, const Diagonal_segment &y)
	{
		return x.subject_end() < y.subject_end();
	}
	static bool cmp_heuristic(const Diagonal_segment &x, const Diagonal_segment &y)
	{
		return (x.subject_end() < y.subject_end() && x.j < y.j)
			|| (x.j - y.j < y.subject_end() - x.subject_end());
	}
	friend int abs_shift(const Diagonal_segment &x, const Diagonal_segment &y)
	{
		return abs(x.diag() - y.diag());
	}
	friend std::ostream& operator<<(std::ostream &s, const Diagonal_segment &d)
	{
		s << "i=" << d.i << " j=" << d.j << " l=" << d.len << " score=" << d.score;
		return s;
	}
	int i, j, len, score;
};

struct DiagonalSegment
{
	DiagonalSegment():
		len(0)
	{}

	DiagonalSegment(const TranslatedPosition &i, int j, int len, int score=0):
		i(i),
		j(j),
		len(len),
		score(score)
	{}

	int subject_last() const
	{
		return j + len - 1;
	}

	TranslatedPosition query_last() const
	{
		return i + len - 1;
	}

	int subject_end() const
	{
		return j + len;
	}

	TranslatedPosition query_end() const
	{
		return i + len;
	}

	int diag() const
	{
		return i - j;
	}

	DiagonalSegment& set_score(const TranslatedSequence &query, const sequence &subject)
	{
		TranslatedPosition i1 = i;
		int j1 = j;
		for (; j1 < subject_end(); ++j1)
			score += score_matrix(query[i1], subject[j1]);
		return *this;
	}

	int splice_score(const DiagonalSegment &d, int gap_open, int gap_extend, int frame_shift) const
	{
		TranslatedPosition i0 = query_last();
		int j0 = subject_last();
		const int fs = i0.frame_shift(d.i);
		if (fs == 1)
			i0.shift_forward();
		else if (fs == -1)
			i0.shift_back();
		const int shift = d.diag() - (i0 - j0),
			penalty = abs(fs)*frame_shift;
		if (shift > 0)
			i0 = i0 + shift;
		else if (shift < 0)
			j0 += shift;
		if (i0 + 1 != d.i || j0 + 1 != d.j)
			throw std::runtime_error("Splice error");
		if (shift != 0)
			return -gap_open - abs(shift)*gap_extend - penalty;
		else
			return -penalty;
	}

	friend std::ostream& operator<<(std::ostream &s, const DiagonalSegment &d)
	{
		s << "i=(" << d.i << ") j=" << d.j << " len=" << d.len << " score=" << d.score << std::endl;
		return s;
	}

	TranslatedPosition i;
	int j, len, score;
};

#endif