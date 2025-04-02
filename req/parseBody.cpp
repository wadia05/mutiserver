#include "HTTPRequest.hpp"

void HTTPRequest::parseURLEncodedBody(std::string body, const std::string &CT)
{
	std::vector<std::map<std::string, std::string> > data;
	std::istringstream iss(body);
	std::string key_value;
	while (std::getline(iss, key_value, '&'))
	{
		size_t pos = key_value.find('=');
		std::map<std::string, std::string> key_value_map;
		if (pos != std::string::npos)
		{
			std::string key = key_value.substr(0, pos);
			std::string value = key_value.substr(pos + 1);
			if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
				value = value.substr(1, value.size() - 2);
			key_value_map[urlDecode(key)] = urlDecode(value);
		}
		else
			key_value_map[urlDecode(key_value)] = "";
		data.push_back(key_value_map);
	}
	std::string www_form_urlencoded = "";
	for (std::vector<std::map<std::string, std::string> >::iterator it = data.begin(); it != data.end(); ++it)
	{
		std::map<std::string, std::string> key_value_map = *it;
		for (std::map<std::string, std::string>::iterator kv_it = key_value_map.begin(); kv_it != key_value_map.end(); ++kv_it)
			www_form_urlencoded += kv_it->first + "=" + kv_it->second + "&";
	}
	if (!www_form_urlencoded.empty())
		www_form_urlencoded = www_form_urlencoded.substr(0, www_form_urlencoded.size() - 1);
	all_body = www_form_urlencoded;
	content_type = CT;
	is_multi_part = 0;
}

void HTTPRequest::parseRawBody(std::string body, const std::string &CT)
{
	all_body = body;
	if (CT.empty())
		content_type = "text/plain";
	else
		content_type = CT;
	is_multi_part = 0;
}

void HTTPRequest::parsePart(const std::string &part_content)
{
	BodyPart part;
	size_t header_end = part_content.find("\r\n\r\n");
	if (header_end == std::string::npos)
	{
		header_end = part_content.find("\n\n");
		if (header_end == std::string::npos)
			return;
	}
	std::string headers_section;
	if (header_end != std::string::npos)
		headers_section = part_content.substr(0, header_end);
	else
		headers_section = part_content;
	std::istringstream headers_stream(headers_section);
	std::string header_line;
	std::string name;
	std::string filename;
	while (std::getline(headers_stream, header_line))
	{
		if (!header_line.empty() && header_line[header_line.size() - 1] == '\r')
			header_line.erase(header_line.size() - 1);
		if (header_line.empty())
		{
			print_message("Empty header line", RED);
			return;
		}
		if (header_line.substr(0, 20) == "Content-Disposition:")
		{
			std::string value = header_line.substr(20);
			size_t name_pos = value.find("name=\"");
			if (name_pos != std::string::npos)
			{
				name_pos += 6;
				size_t name_end = value.find('"', name_pos);
				if (name_end != std::string::npos)
					name = value.substr(name_pos, name_end - name_pos);
				part.name = name;
			}
			size_t filename_pos = value.find("filename=\"");
			if (filename_pos != std::string::npos)
			{
				filename_pos += 10;
				size_t filename_end = value.find('"', filename_pos);
				part.is_file = 1;
				if (filename_end != std::string::npos)
					filename = value.substr(filename_pos, filename_end - filename_pos);
				part.filename = filename;
				if (filename.empty())
					part.filename = "filename";
			}
			size_t first_semicolon = value.find(";");
			if (first_semicolon != std::string::npos)
			{
				std::string content_type = value.substr(0, first_semicolon);
				trim(content_type);
				part.content_type = content_type;
			}
		}
		else if (header_line.substr(0, 13) == "Content-Type:")
		{
			part.type_of_file = header_line.substr(13);
			trim(part.content_type);
			if (part.content_type.empty())
				part.content_type = "text/plain";
		}
		else
		{
			size_t colon_pos = header_line.find(':');
			if (colon_pos != std::string::npos)
			{
				std::string value = header_line.substr(colon_pos + 1);
				trim(value);
				part.content_type = value;
			}
		}
	}
	std::string body_content;
	if (header_end != std::string::npos)
	{
		size_t body_start;
		if (part_content.substr(header_end, 4) == "\r\n\r\n")
			body_start = header_end + 4;
		else
			body_start = header_end + 2;
		body_content = part_content.substr(body_start);
		if (body_content.length() >= 2 && body_content.substr(body_content.length() - 2) == "\r\n")
			body_content = body_content.substr(0, body_content.length() - 2);
	}
	else
		body_content = part_content.substr(header_end);
	part.data = body_content;
	body_parts.push_back(part);
}

void HTTPRequest::parseMultipartBody(std::string body, const std::string &CT)
{
	size_t boundary_pos = CT.find("boundary=");
	if (boundary_pos == std::string::npos)
	{
		parseRawBody(body, CT);
		return;
	}
	boundary_pos += 9;
	std::string boundary_content = CT.substr(boundary_pos);
	std::string boundary = "--" + boundary_content;
	std::string end_boundary = boundary + "--";
	size_t start_pos = body.find(boundary);
	if (start_pos == std::string::npos)
	{
		parseRawBody(body, CT);
		return;
	}
	this->all_body = body.substr(start_pos);
	start_pos += boundary.length();
	while (start_pos < body.length())
	{
		if (body.substr(start_pos, 2) == "\r\n")
			start_pos += 2;
		else if (body[start_pos] == '\n')
			start_pos += 1;
		if (start_pos + end_boundary.length() <= body.length() &&
			body.substr(start_pos, end_boundary.length()) == end_boundary)
			break;
		size_t next_boundary = body.find(boundary, start_pos);
		if (next_boundary == std::string::npos)
			break;
		std::string part_content = body.substr(start_pos, next_boundary - start_pos);
		parsePart(part_content);
		start_pos = next_boundary + boundary.length();
	}
	is_multi_part = 1;
}
bool HTTPRequest::parseBody(std::istringstream &iss)
{
	std::string line;
	std::string raw_body;
	while (std::getline(iss, line))
		raw_body += line + "\n";
	if (raw_body.empty())
		return true;
	if (raw_body[raw_body.length() - 1] == '\n')
		raw_body = raw_body.substr(0, raw_body.length() - 1);
	this->all_body = raw_body;
	std::string contentType = getHeader("content-type");
	if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
		parseURLEncodedBody(raw_body, contentType);
	else if (contentType.find("multipart/form-data") != std::string::npos)
		parseMultipartBody(raw_body, contentType);
	else
		parseRawBody(raw_body, contentType);
	return true;
}