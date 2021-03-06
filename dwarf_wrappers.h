// -*- C++ -*-
// Copyright (C) 2008-2011 Red Hat Inc.
//
// This file is part of systemtap, and is free software.  You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#ifndef DWARF_WRAPPERS_H
#define DWARF_WRAPPERS_H 1

#include "config.h"

extern "C" {
#include <elfutils/libdw.h>
#include <elfutils/version.h>
#include <dwarf.h>
}

#include <string>

#if ! _ELFUTILS_PREREQ(0, 148)
#define DW_AT_linkage_name 0x6e
#endif


// NB: "rc == 0" means OK in this case
void dwfl_assert(const std::string& desc, int rc);

// Throw error if pointer is NULL
inline void
dwfl_assert(const std::string& desc, const void* ptr)
{
  if (!ptr)
    dwfl_assert(desc, -1);
}

// Throw error if condition is false
inline void
dwfl_assert(const std::string& desc, bool condition)
{
  if (!condition)
    dwfl_assert(desc, -1);
}

// NB: "rc == 0" means OK in this case
void dwarf_assert(const std::string& desc, int rc);

// Throw error if pointer is NULL
inline void
dwarf_assert(const std::string& desc, const void* ptr)
{
  if (!ptr)
    dwarf_assert(desc, -1);
}


class dwarf_line_t
{
public:
  const Dwarf_Line* line;
  dwarf_line_t() : line(0) {}
  dwarf_line_t(const Dwarf_Line* line_) : line(line_) {}

  dwarf_line_t& operator= (const Dwarf_Line* line_)
  {
    line = (line_);
    return *this;
  }

  operator bool() const
  {
    return line != 0;
  }

  int lineno() const
  {
    int lineval;
    if (!line)
      dwarf_assert("dwarf_line_t::lineno", -1);
    dwarf_lineno(const_cast<Dwarf_Line*>(line), &lineval);
    return lineval;
  }
  Dwarf_Addr addr() const
  {
    Dwarf_Addr addrval;
    if (!line)
      dwarf_assert("dwarf_line_t::addr", -1);
    dwarf_lineaddr(const_cast<Dwarf_Line*>(line), &addrval);
    return addrval;
  }
  const char* linesrc(Dwarf_Word* mtime = 0, Dwarf_Word* length = 0)
  {
    const char* retval = dwarf_linesrc(const_cast<Dwarf_Line*>(line), mtime,
                                                               length);
    dwarf_assert("dwarf_line_t::linesrc", retval);
    return retval;
  }
};


// Look up the DIE for a reference-form attribute name
inline Dwarf_Die *
dwarf_attr_die (Dwarf_Die *die, unsigned int attr, Dwarf_Die *result)
{
  Dwarf_Attribute attr_mem;
  return dwarf_formref_die (dwarf_attr_integrate (die, attr, &attr_mem),
                            result);
}


// Retrieve the linkage name of a die, either by the MIPS vendor extension or
// DWARF4's standardized attribute.
inline const char *
dwarf_linkage_name (Dwarf_Die *die)
{
  Dwarf_Attribute attr_mem;
  return dwarf_formstring
    (dwarf_attr_integrate (die, DW_AT_MIPS_linkage_name, &attr_mem)
     ?: dwarf_attr_integrate (die, DW_AT_linkage_name, &attr_mem));
}


#if !_ELFUTILS_PREREQ(0, 143)
// Elfutils prior to 0.143 didn't use attr_integrate when looking up the
// decl_file or decl_line, so the attributes would sometimes be missed.  For
// those old versions, we define custom implementations to do the integration.

const char *dwarf_decl_file_integrate (Dwarf_Die *die);
#define dwarf_decl_file dwarf_decl_file_integrate

int dwarf_decl_line_integrate (Dwarf_Die *die, int *linep)
  __nonnull_attribute__ (2);
#define dwarf_decl_line dwarf_decl_line_integrate

#endif // !_ELFUTILS_PREREQ(0, 143)


// Resolve a full name for dwarf types
bool dwarf_type_name(Dwarf_Die *type_die, std::string& type_name);
std::string dwarf_type_name(Dwarf_Die *type_die);


#endif

/* vim: set sw=2 ts=8 cino=>4,n-2,{2,^-2,t0,(0,u0,w1,M1 : */
