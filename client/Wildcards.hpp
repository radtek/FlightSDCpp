/*
  * Copyright (c) 2013 Developed by reg <entry.reg@gmail.com>
  * 
  * Distributed under the Boost Software License, Version 1.0
  * (see accompanying file LICENSE or a copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include "typedefs.h" //https://bitbucket.org/3F/sandbox/src/ee1d809bcb905c1b01cea8c0e595337dab6d3d27/cpp/text/wildcards/wildcards/typedefs.hpp

using namespace dcpp;

namespace reg { namespace text { namespace wildcards {

    class Wildcards
    {
    public:

        enum MetaOperation{
            FLUSH   = 0,
            ANY     = 1,
            ANYSP   = 2,
            SPLIT   = 4,
            ONE     = 8,
            START   = 16,
            END     = 32,
            EOL     = 64,
        };

        enum MetaSymbols{
            MS_ANY      = _T('*'),
            MS_ANYSP    = _T('>'), //as [^/\\]+
            MS_SPLIT    = _T('|'),
            MS_ONE      = _T('?'),
            MS_START    = _T('^'),
            MS_END      = _T('$'),
        };

        bool main(const tstring& text, const tstring& filter);

        Wildcards(void)
        {
            _asserts();
        };
        ~Wildcards(void){};

    protected:
        /** verify MetaOperation::ANY */
        void _assertsAny();
        /** verify MetaOperation::SPLIT */
        void _assertsSplit();
        /** verify MetaOperation::ONE */
        void _assertsOne();
        /** verify MetaOperation::ANYSP */
        void _assertsAnySP();
        /** wrapper */
        void _asserts();

        struct Mask{
            MetaOperation curr;
            MetaOperation prev;
            Mask(): curr(FLUSH), prev(FLUSH){};
        };

        /**
         * to wildcards
         */
        struct Item{
            tstring curr;
            size_t pos;
            size_t left;
            size_t delta;
            Mask mask;
            tstring prev;
            Item(): pos(0), left(0), delta(0){};
        };

        /**
         * to words
         */
        struct Words{
            size_t found;
            size_t left;
            Words(): left(0){};
        };

        /**
         * Working with an interval:
         *      _______
         * {word} ... {word}
         */
        size_t _handlerInterval(Item& item, Words& words, const tstring& text);
    };

}}};