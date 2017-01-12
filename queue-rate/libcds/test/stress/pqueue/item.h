/*
    This file is a part of libcds - Concurrent Data Structures library

    (C) Copyright Maxim Khizhinsky (libcds.dev@gmail.com) 2006-2016

    Source code repo: http://github.com/khizmax/libcds/
    Download: http://sourceforge.net/projects/libcds/files/
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     
*/

#ifndef CDSSTRESS_PQUEUE_ITEM_H
#define CDSSTRESS_PQUEUE_ITEM_H

namespace pqueue {

    struct simple_value {
        typedef size_t  key_type;

        key_type key;

        struct key_extractor {
            void operator()( key_type& k, simple_value const& s ) const
            {
                k = s.key;
            }
        };

        simple_value()
            : key(0)
        {}

        simple_value( key_type n )
            : key(n) 
        {}
    };
} // namespace pqueue

namespace std {
    template <class T> struct less;
    template <class T> struct greater;

    template <>
    struct less<pqueue::simple_value>
    {
        bool operator()( pqueue::simple_value const& k1, pqueue::simple_value const& k2 ) const
        {
            return k1.key < k2.key;
        }

        bool operator()( pqueue::simple_value const& k1, size_t k2 ) const
        {
            return k1.key < k2;
        }

        bool operator()( size_t k1, pqueue::simple_value const& k2 ) const
        {
            return k1 < k2.key;
        }

        bool operator()( size_t k1, size_t k2 ) const
        {
            return k1 < k2;
        }
    };

    template <>
    struct greater<pqueue::simple_value>
    {
        bool operator()( pqueue::simple_value const& k1, pqueue::simple_value const& k2 ) const
        {
            return k1.key > k2.key;
        }

        bool operator()( pqueue::simple_value const& k1, size_t k2 ) const
        {
            return k1.key > k2;
        }

        bool operator()( size_t k1, pqueue::simple_value const& k2 ) const
        {
            return k1 > k2.key;
        }

        bool operator()( size_t k1, size_t k2 ) const
        {
            return k1 > k2;
        }
    };

} // namespace std

#endif // #ifndef CDSSTRESS_PQUEUE_ITEM_H
