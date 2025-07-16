#include "urlparser.h"

#include "stddef.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

/*
	Free memory of parsed url
*/
void parsed_url_free(struct parsed_url *purl)
{
    if ( NULL != purl )
	{
        if ( NULL != purl->scheme ) xy_free(purl->scheme);
        if ( NULL != purl->host ) xy_free(purl->host);
//        if ( NULL != purl->port ) xy_free(purl->port);
        if ( NULL != purl->path )  xy_free(purl->path);
        if ( NULL != purl->query ) xy_free(purl->query);
        if ( NULL != purl->fragment ) xy_free(purl->fragment);
        if ( NULL != purl->username ) xy_free(purl->username);
        if ( NULL != purl->password ) xy_free(purl->password);
        xy_free(purl);
    }
}

/*
	Retrieves the IP adress of a hostname
*/
int hostname_to_ip(char *hostname, char *ip, int ip_len)
{
//	char *ip = "0.0.0.0";
	struct hostent *h;\
	int ret = 0;
	struct hostent hostent_ret = {0};
	struct hostent *hostent_result = NULL;
	char temp_buf[100] = {0};
	int h_errno = 0;
	ret = lwip_gethostbyname_r(hostname, &hostent_ret, temp_buf, 100, &hostent_result, &h_errno);
	if (ret < 0)
	{
		xy_printf("gethostbyname failed,ret:%d,h_errno:%d", ret, h_errno);
		return -1;
	}
	inet_ntoa_r(*((struct in_addr *)hostent_result->h_addr), ip, ip_len);
	return 0;
}

/*
	Check whether the character is permitted in scheme string
*/
int is_scheme_char(int c)
{
    return (!isalpha(c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
}

/*
	Parses a specified URL and returns the structure named 'parsed_url'
	Implented according to:
	RFC 1738 - http://www.ietf.org/rfc/rfc1738.txt
	RFC 3986 -  http://www.ietf.org/rfc/rfc3986.txt
*/
struct parsed_url *parse_url(const char *url)
{

	/* Define variable */
    struct parsed_url *purl;
    const char *tmpstr;
    const char *curstr;
    int len;
    int i;
    int userpass_flag;
    int bracket_flag;

    /* Allocate the parsed url storage */
    purl = (struct parsed_url*)xy_zalloc(sizeof(struct parsed_url));
    if ( NULL == purl )
	{
        return NULL;
    }
    purl->scheme = NULL;
    purl->host = NULL;
//    purl->port = NULL;
    purl->path = NULL;
    purl->query = NULL;
    purl->fragment = NULL;
    purl->username = NULL;
    purl->password = NULL;
    curstr = url;
#if 0
    /*
     * <scheme>:<scheme-specific-part>
     * <scheme> := [a-z\+\-\.]+
     *             upper case = lower case for resiliency
     */
    /* Read scheme */
    tmpstr = strchr(curstr, ':');
    if ( NULL == tmpstr )
	{
        parsed_url_free(purl); //fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);

        return NULL;
    }

    /* Get the scheme length */
    len = tmpstr - curstr;

    /* Check restrictions */
    for ( i = 0; i < len; i++ )
	{
        if (is_scheme_char(curstr[i]) == 0)
		{
            /* Invalid format */
            parsed_url_free(purl); //fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
    }
    /* Copy the scheme to the storage */
    purl->scheme = (char*)xy_zalloc(sizeof(char) * (len + 1));
    if ( NULL == purl->scheme )
	{
        parsed_url_free(purl); //fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);

        return NULL;
    }

    (void)strncpy(purl->scheme, curstr, len);
    purl->scheme[len] = '\0';

    /* Make the character to lower if it is upper case. */
    for ( i = 0; i < len; i++ )
	{
        purl->scheme[i] = tolower(purl->scheme[i]);
    }

    /* Skip ':' */
    tmpstr++;
    curstr = tmpstr;

    /*
     * //<user>:<password>@<host>:<port>/<url-path>
     * Any ":", "@" and "/" must be encoded.
     */
    /* Eat "//" */
    for ( i = 0; i < 2; i++ )
	{
        if ( '/' != *curstr )
		{
            parsed_url_free(purl); //fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        curstr++;
    }
#endif
    purl->scheme = (char*)xy_zalloc(strlen("http") + 1);
	if ( NULL == purl->scheme )
	{
		parsed_url_free(purl);

		return NULL;
	}
	strcpy(purl->scheme, "http");
    /* Check if the user (and password) are specified. */
    userpass_flag = 0;
    tmpstr = curstr;
    while ( '\0' != *tmpstr )
	{
        if ( '@' == *tmpstr )
		{
            /* Username and password are specified */
            userpass_flag = 1;
            break;
        }
		else if ( '/' == *tmpstr )
		{
            /* End of <host>:<port> specification */
            userpass_flag = 0;
            break;
        }
        tmpstr++;
    }

    /* User and password specification */
    tmpstr = curstr;
    if ( userpass_flag )
	{
        /* Read username */
        while ( '\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr )
		{
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->username = (char*)xy_zalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->username )
		{
            parsed_url_free(purl); //fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->username, curstr, len);
        purl->username[len] = '\0';

        /* Proceed current pointer */
        curstr = tmpstr;
        if ( ':' == *curstr )
		{
            /* Skip ':' */
            curstr++;

            /* Read password */
            tmpstr = curstr;
            while ( '\0' != *tmpstr && '@' != *tmpstr )
			{
                tmpstr++;
            }
            len = tmpstr - curstr;
            purl->password = (char*)xy_zalloc(sizeof(char) * (len + 1));
            if ( NULL == purl->password )
			{
                parsed_url_free(purl); //fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
                return NULL;
            }
            (void)strncpy(purl->password, curstr, len);
            purl->password[len] = '\0';
            curstr = tmpstr;
        }
        /* Skip '@' */
        if ( '@' != *curstr )
		{
            parsed_url_free(purl); ////fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        curstr++;
    }

    if ( '[' == *curstr )
	{
        bracket_flag = 1;
    }
	else
	{
        bracket_flag = 0;
    }
    /* Proceed on by delimiters with reading host */
    tmpstr = curstr;
    while ( '\0' != *tmpstr ) {
        if ( bracket_flag && ']' == *tmpstr )
 		{
            /* End of IPv6 address. */
            tmpstr++;
            break;
        }
		else if ( !bracket_flag && (':' == *tmpstr || '/' == *tmpstr) )
		{
            /* Port number is specified. */
            break;
        }
        tmpstr++;
    }
    len = tmpstr - curstr;
    purl->host = (char*)xy_zalloc(sizeof(char) * (len + 1));
    if ( NULL == purl->host || len <= 0 )
	{
        parsed_url_free(purl); ////fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }
    (void)strncpy(purl->host, curstr, len);
    purl->host[len] = '\0';
    curstr = tmpstr;

    /* Is port number specified? */
    if ( ':' == *curstr )
	{
        curstr++;
        /* Read port number */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '/' != *tmpstr )
		{
            tmpstr++;
        }
        len = tmpstr - curstr;
        (void)strncpy(purl->port, curstr, len);
        purl->port[len] = '\0';
        curstr = tmpstr;
    }
	else
	{
//		strcpy(purl->port, "80");
	}

	/* Get ip */
	if(hostname_to_ip(purl->host, purl->ip, 16) < 0)
	{
		parsed_url_free(purl);
		return NULL;
	}

	/* Set uri */
	purl->uri = (char*)url;

    /* End of the string */
    if ( '\0' == *curstr )
	{
        return purl;
    }

    /* Skip '/' */
    if ( '/' != *curstr )
	{
        parsed_url_free(purl); ////fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }
    curstr++;

    /* Parse path */
    tmpstr = curstr;
    while ( '\0' != *tmpstr && '#' != *tmpstr  && '?' != *tmpstr )
	{
        tmpstr++;
    }
    len = tmpstr - curstr;
    purl->path = (char*)xy_zalloc(sizeof(char) * (len + 1));
    if ( NULL == purl->path )
	{
        parsed_url_free(purl); ////fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }
    (void)strncpy(purl->path, curstr, len);
    purl->path[len] = '\0';
    curstr = tmpstr;

    /* Is query specified? */
    if ( '?' == *curstr )
	{
        /* Skip '?' */
        curstr++;
        /* Read query */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '#' != *tmpstr )
		{
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->query = (char*)xy_zalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->query )
		{
            parsed_url_free(purl); ////fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->query, curstr, len);
        purl->query[len] = '\0';
        curstr = tmpstr;
    }

    /* Is fragment specified? */
    if ( '#' == *curstr )
	{
        /* Skip '#' */
        curstr++;
        /* Read fragment */
        tmpstr = curstr;
        while ( '\0' != *tmpstr )
		{
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->fragment = (char*)xy_zalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->fragment )
 		{
            parsed_url_free(purl); ////fprintf(stderr, "Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->fragment, curstr, len);
        purl->fragment[len] = '\0';
        curstr = tmpstr;
    }
	return purl;
}
