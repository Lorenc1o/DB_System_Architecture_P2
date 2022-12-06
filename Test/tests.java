package org.example;
import java.net.*;
import java.util.ArrayList;
import java.util.List;


public class Main {
    public static void main(String[] args) {
        try{
            /*-- Exchanging each element
        insert into test_url values (18, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','https://'));
        insert into test_url values (28, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','www.spec.com'));
        insert into test_url values (38, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','http://www.spec.com/'));
        insert into test_url values (48, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','/filespec/spec/spec2/'));
        insert into test_url values (58, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','?query=testing&spec=2'));
        insert into test_url values (68, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','#specRef'));
        insert into test_url values (78, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','ftp://www.spec.com/filespec/spec?specQ#ref'));*/

            List<URL> listExchangeElement=new ArrayList<URL>();
            URL baseURL1 = new URL("http://www.test.com/file?param1=1&param2=2#contextRef");
            listExchangeElement.add(new URL(baseURL1, "https://"));
            listExchangeElement.add(new URL(baseURL1, "www.spec.com"));
            listExchangeElement.add(new URL(baseURL1, "http://www.spec.com/"));
            listExchangeElement.add(new URL(baseURL1, "/filespec/spec/spec2/"));
            listExchangeElement.add(new URL(baseURL1, "?query=testing&spec=2"));
            listExchangeElement.add(new URL(baseURL1, "#specRef"));
            listExchangeElement.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "?query=testing&spec=2"));
            System.out.println("--Exchanging each element:");
            for (int i = 0; i < listExchangeElement.size(); i++) {
                System.out.println(i+": "+listExchangeElement.get(i));
            }



/*          insert into test_url values (71, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','www.spec.com/filespec/spec?specQ#ref'));
            insert into test_url values (82, pg_url('fttp://www.test.com/file?param1=1&param2=2','http://www.spec.com/')); --F*/
            List<URL> listInherited=new ArrayList<URL>();
            listInherited.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "www.spec.com/filespec/spec?specQ#ref"));
            listInherited.add(new URL(new URL("http://www.test.com/file?param1=1&param2=2"), "http://www.spec.com/"));
            System.out.println("--If spec has no scheme, the scheme component is inherited from the context URL.:");
            for (int i = 0; i < listInherited.size(); i++) {
                System.out.println(i+": "+listInherited.get(i));
            }

/*            --TODO: The reference is parsed into the scheme, authority, path, query and fragment parts.
                 If the path component is empty and the scheme, authority, and query components are undefined,
                 then the new URL is a reference to the current document. Otherwise, the fragment and query parts present in the spec are used in the new URL.
            insert into test_url values (17, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','#ref'));
            insert into test_url values (27, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref',''));
            insert into test_url values (37, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','?query=testing&spec=2#specRef'));*/

            URL baseURL3 = new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref");

            List<URL> listEmptyPath=new ArrayList<URL>();
            listEmptyPath.add(new URL(baseURL3, "#ref"));
            listEmptyPath.add(new URL(baseURL3, ""));
            listEmptyPath.add(new URL(baseURL3, "?query=testing&spec=2#specRef"));
            System.out.println("--TODO: The reference is parsed into the scheme, authority, path, query and fragment parts. If the path component is empty and the scheme, authority, and query components are undefined, then the new URL is a reference to the current document. Otherwise, the fragment and query parts present in the spec are used in the new URL.:");
            for (int i = 0; i < listEmptyPath.size(); i++) {
                System.out.println(i+": "+listEmptyPath.get(i));
            }



/*            --If the scheme component is defined in the given spec and does not match the scheme of the context, then the new URL is created as an absolute URL based on the spec alone. Otherwise the scheme component is inherited from the context URL.

            insert into test_url values (49, pg_url('ftp://www.test.com/file?param1=1&param2=2','http://www.spec.com/')); --F
            insert into test_url values (58, pg_url('ftp://www.test.com/file?param1=1&param2=2','ftp://www.spec.com/')); --F
            insert into test_url values (7, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','www.spec.com/filespec/spec?specQ#specref'));
            select * from test_url;*/

            URL baseURL4 = new URL("ftp://www.test.com/file?param1=1&param2=2");

            List<URL> listDefinedComponent=new ArrayList<URL>();
            listDefinedComponent.add(new URL(baseURL4, "http://www.spec.com/"));
            listDefinedComponent.add(new URL(baseURL4, "ftp://www.spec.com/"));
            listDefinedComponent.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "?query=testing&spec=2#specRef"));
            System.out.println("--If the scheme component is defined in the given spec and does not match the scheme of the context, then the new URL is created as an absolute URL based on the spec alone. Otherwise the scheme component is inherited from the context URL:");
            for (int i = 0; i < listDefinedComponent.size(); i++) {
                System.out.println(i+": "+listDefinedComponent.get(i));
            }




        }catch(Exception e){System.out.println(e);}
    }
}