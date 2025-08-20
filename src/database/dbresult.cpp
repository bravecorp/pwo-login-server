/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <database/dbresult.h>

DBResult::DBResult(MYSQL_RES* res)
{
    size_t i = 0;

    auto field = mysql_fetch_field(res);
    while (field) {
        m_fields[field->name] = i++;
        field = mysql_fetch_field(res);
    }

    auto row = mysql_fetch_row(res);
    while (row) {
        m_rows.push_back(row);
        row = mysql_fetch_row(res);
    }
}

std::string DBResult::getString(const std::string& field)
{
    if (m_rows.empty() || m_rows.size() < m_index) {
        return "";
    }

    auto it = m_fields.find(field);
    if (it == m_fields.end()) {
        // field name doesnt exist
        return "";
    }

    auto row = m_rows[m_index];
    if (row[it->second] == nullptr) {
        return "";
    }

    return std::string(row[it->second]);
}
