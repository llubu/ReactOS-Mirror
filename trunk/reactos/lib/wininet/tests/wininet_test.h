/*
 * Unit test suite for wininet functions
 *
 * Copyright 2004 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* The Windows headers don't define A/W types for any of the following
 * structures and pointers :-(
 * Because for these structures there's not alignment or packing difference
 * between the A and W versions, we just define a set of macros so the
 * generated tests work anyway.
 */
#ifndef GOPHER_ABSTRACT_ATTRIBUTE_TYPE
#define GOPHER_ABSTRACT_ATTRIBUTE_TYPEA GOPHER_ABSTRACT_ATTRIBUTE_TYPE
#define GOPHER_ABSTRACT_ATTRIBUTE_TYPEW GOPHER_ABSTRACT_ATTRIBUTE_TYPE

#define GOPHER_ADMIN_ATTRIBUTE_TYPEA GOPHER_ADMIN_ATTRIBUTE_TYPE
#define GOPHER_ADMIN_ATTRIBUTE_TYPEW GOPHER_ADMIN_ATTRIBUTE_TYPE
#define GOPHER_ASK_ATTRIBUTE_TYPEA GOPHER_ASK_ATTRIBUTE_TYPE
#define GOPHER_ASK_ATTRIBUTE_TYPEW GOPHER_ASK_ATTRIBUTE_TYPE
#define GOPHER_ATTRIBUTE_ENUMERATORA GOPHER_ATTRIBUTE_ENUMERATOR
#define GOPHER_ATTRIBUTE_ENUMERATORW GOPHER_ATTRIBUTE_ENUMERATOR
#define GOPHER_ATTRIBUTE_TYPEA GOPHER_ATTRIBUTE_TYPE
#define GOPHER_ATTRIBUTE_TYPEW GOPHER_ATTRIBUTE_TYPE
#define GOPHER_LOCATION_ATTRIBUTE_TYPEA GOPHER_LOCATION_ATTRIBUTE_TYPE
#define GOPHER_LOCATION_ATTRIBUTE_TYPEW GOPHER_LOCATION_ATTRIBUTE_TYPE
#define GOPHER_ORGANIZATION_ATTRIBUTE_TYPEA GOPHER_ORGANIZATION_ATTRIBUTE_TYPE
#define GOPHER_ORGANIZATION_ATTRIBUTE_TYPEW GOPHER_ORGANIZATION_ATTRIBUTE_TYPE
#define GOPHER_PROVIDER_ATTRIBUTE_TYPEA GOPHER_PROVIDER_ATTRIBUTE_TYPE
#define GOPHER_PROVIDER_ATTRIBUTE_TYPEW GOPHER_PROVIDER_ATTRIBUTE_TYPE
#define GOPHER_SITE_ATTRIBUTE_TYPEA GOPHER_SITE_ATTRIBUTE_TYPE
#define GOPHER_SITE_ATTRIBUTE_TYPEW GOPHER_SITE_ATTRIBUTE_TYPE
#define GOPHER_UNKNOWN_ATTRIBUTE_TYPEA GOPHER_UNKNOWN_ATTRIBUTE_TYPE
#define GOPHER_UNKNOWN_ATTRIBUTE_TYPEW GOPHER_UNKNOWN_ATTRIBUTE_TYPE
#define GOPHER_VERSION_ATTRIBUTE_TYPEA GOPHER_VERSION_ATTRIBUTE_TYPE
#define GOPHER_VERSION_ATTRIBUTE_TYPEW GOPHER_VERSION_ATTRIBUTE_TYPE
#define GOPHER_VIEW_ATTRIBUTE_TYPEA GOPHER_VIEW_ATTRIBUTE_TYPE
#define GOPHER_VIEW_ATTRIBUTE_TYPEW GOPHER_VIEW_ATTRIBUTE_TYPE
#define INTERNET_CERTIFICATE_INFOA INTERNET_CERTIFICATE_INFO
#define INTERNET_CERTIFICATE_INFOW INTERNET_CERTIFICATE_INFO
#define INTERNET_PROXY_INFOA INTERNET_PROXY_INFO
#define INTERNET_PROXY_INFOW INTERNET_PROXY_INFO

#define LPGOPHER_ABSTRACT_ATTRIBUTE_TYPEA LPGOPHER_ABSTRACT_ATTRIBUTE_TYPE
#define LPGOPHER_ABSTRACT_ATTRIBUTE_TYPEW LPGOPHER_ABSTRACT_ATTRIBUTE_TYPE
#define LPGOPHER_ADMIN_ATTRIBUTE_TYPEA LPGOPHER_ADMIN_ATTRIBUTE_TYPE
#define LPGOPHER_ADMIN_ATTRIBUTE_TYPEW LPGOPHER_ADMIN_ATTRIBUTE_TYPE
#define LPGOPHER_ASK_ATTRIBUTE_TYPEA LPGOPHER_ASK_ATTRIBUTE_TYPE
#define LPGOPHER_ASK_ATTRIBUTE_TYPEW LPGOPHER_ASK_ATTRIBUTE_TYPE
#define LPGOPHER_ATTRIBUTE_TYPEA LPGOPHER_ATTRIBUTE_TYPE
#define LPGOPHER_ATTRIBUTE_TYPEW LPGOPHER_ATTRIBUTE_TYPE
#define LPGOPHER_LOCATION_ATTRIBUTE_TYPEA LPGOPHER_LOCATION_ATTRIBUTE_TYPE
#define LPGOPHER_LOCATION_ATTRIBUTE_TYPEW LPGOPHER_LOCATION_ATTRIBUTE_TYPE
#define LPGOPHER_ORGANIZATION_ATTRIBUTE_TYPEA LPGOPHER_ORGANIZATION_ATTRIBUTE_TYPE
#define LPGOPHER_ORGANIZATION_ATTRIBUTE_TYPEW LPGOPHER_ORGANIZATION_ATTRIBUTE_TYPE
#define LPGOPHER_PROVIDER_ATTRIBUTE_TYPEA LPGOPHER_PROVIDER_ATTRIBUTE_TYPE
#define LPGOPHER_PROVIDER_ATTRIBUTE_TYPEW LPGOPHER_PROVIDER_ATTRIBUTE_TYPE
#define LPGOPHER_SITE_ATTRIBUTE_TYPEA LPGOPHER_SITE_ATTRIBUTE_TYPE
#define LPGOPHER_SITE_ATTRIBUTE_TYPEW LPGOPHER_SITE_ATTRIBUTE_TYPE
#define LPGOPHER_UNKNOWN_ATTRIBUTE_TYPEA LPGOPHER_UNKNOWN_ATTRIBUTE_TYPE
#define LPGOPHER_UNKNOWN_ATTRIBUTE_TYPEW LPGOPHER_UNKNOWN_ATTRIBUTE_TYPE
#define LPGOPHER_VERSION_ATTRIBUTE_TYPEA LPGOPHER_VERSION_ATTRIBUTE_TYPE
#define LPGOPHER_VERSION_ATTRIBUTE_TYPEW LPGOPHER_VERSION_ATTRIBUTE_TYPE
#define LPGOPHER_VIEW_ATTRIBUTE_TYPEA LPGOPHER_VIEW_ATTRIBUTE_TYPE
#define LPGOPHER_VIEW_ATTRIBUTE_TYPEW LPGOPHER_VIEW_ATTRIBUTE_TYPE
#define LPINTERNET_CERTIFICATE_INFOA LPINTERNET_CERTIFICATE_INFO
#define LPINTERNET_CERTIFICATE_INFOW LPINTERNET_CERTIFICATE_INFO
#define LPINTERNET_PROXY_INFOA LPINTERNET_PROXY_INFO
#define LPINTERNET_PROXY_INFOW LPINTERNET_PROXY_INFO
#endif
