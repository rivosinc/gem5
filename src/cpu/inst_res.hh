/*
 * Copyright (c) 2016-2017 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CPU_INST_RES_HH__
#define __CPU_INST_RES_HH__

#include <cstdint>
#include <cstring>
#include <string>

#include "base/logging.hh"
#include "base/types.hh"
#include "cpu/reg_class.hh"

namespace gem5
{

class InstResult
{
  private:
    enum
    {
        Valid = 0x1,
        Blob = 0x2
    };
    uint8_t flags = 0;

    bool valid() const { return flags & Valid; }
    void
    valid(bool val)
    {
        if (val)
            flags |= Valid;
        else
            flags &= ~Valid;
    }

    bool blob() const { return flags & Blob; }
    void
    blob(bool val)
    {
        if (val)
            flags |= Blob;
        else
            flags &= ~Blob;
    }

    const RegClass *_regClass = nullptr;
    union Value
    {
        RegVal reg;
        const uint8_t *bytes = nullptr;
    } value;

    void
    setBytes(const void *val)
    {
        const size_t size = _regClass->regBytes();
        if (blob())
            delete[] value.bytes;
        uint8_t *temp = new uint8_t[size];
        std::memcpy(temp, val, size);
        value.bytes = temp;
        blob(true);
    }

    void
    setRegVal(RegVal val)
    {
        if (blob())
            delete[] value.bytes;
        blob(false);
        value.reg = val;
    }

  public:
    /** Default constructor creates an invalid result. */
    InstResult() {}
    InstResult(const InstResult &) = default;

    InstResult(const RegClass &reg_class, RegVal val) : _regClass(&reg_class)
    {
        valid(true);
        setRegVal(val);
    }
    InstResult(const RegClass &reg_class, const void *val) :
        _regClass(&reg_class)
    {
        valid(true);
        setBytes(val);
    }

    virtual ~InstResult()
    {
        if (blob())
            delete[] value.bytes;
    }

    InstResult &
    operator=(const InstResult& that)
    {
        valid(that.valid());
        _regClass = that._regClass;

        if (valid()) {
            if (that.blob())
                setBytes(that.value.bytes);
            else
                setRegVal(that.value.reg);
        }

        return *this;
    }

    /**
     * Result comparison
     * Two invalid results always differ.
     */
    bool
    operator==(const InstResult& that) const
    {
        // If we're not valid, they're equal to us iff they are also not valid.
        if (!valid())
            return !that.valid();

        // If both are valid, check if th register class and flags.
        if (_regClass != that._regClass || flags != that.flags)
            return false;

        // Check our data based on whether we store a blob or a RegVal.
        if (blob()) {
            return std::memcmp(value.bytes, that.value.bytes,
                    _regClass->regBytes()) == 0;
        } else {
            return value.reg == that.value.reg;
        }
    }

    bool
    operator!=(const InstResult& that) const
    {
        return !operator==(that);
    }

    const RegClass &regClass() const { return *_regClass; }
    bool isValid() const { return valid(); }
    bool isBlob() const { return blob(); }

    RegVal
    asRegVal() const
    {
        assert(!blob());
        return value.reg;
    }

    const void *
    asBlob() const
    {
        assert(blob());
        return value.bytes;
    }

    std::string
    asString() const
    {
        if (blob())
            return _regClass->valString(value.bytes);
        else
            return _regClass->valString(&value.reg);
    }
};

} // namespace gem5

#endif // __CPU_INST_RES_HH__
