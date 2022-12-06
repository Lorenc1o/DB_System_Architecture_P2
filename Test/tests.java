package org.example;
import java.net.*;
import java.util.ArrayList;
import java.util.List;


public class Main {
    public static void main(String[] args) {
        //--Exchanging each element:
        try {
            List<URL> listExchangeElement = new ArrayList<URL>();
            URL baseURL1 = new URL("ftp://www.test.com/file?param1=1&param2=2#contextRef");
            listExchangeElement.add(new URL(baseURL1, "https://"));
            listExchangeElement.add(new URL(baseURL1, "www.spec.com"));
            listExchangeElement.add(new URL(baseURL1, "http://www.spec.com/"));
            listExchangeElement.add(new URL(baseURL1, "/filespec/spec/spec2/"));
            listExchangeElement.add(new URL(baseURL1, "?query=testing&spec=2"));
            listExchangeElement.add(new URL(baseURL1, "#specRef"));
            listExchangeElement.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "?query=testing&spec=2"));

            System.out.println("\n--Exchanging each element:");
            for (int i = 0; i < listExchangeElement.size(); i++) {
                System.out.println(i + ": " + listExchangeElement.get(i));
            }
        }catch(Exception e){System.out.println(e);}

        //--If spec has no scheme,
        try {
            List<URL> listInherited = new ArrayList<URL>();
            listInherited.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "www.spec.com/filespec/spec?specQ#ref"));
            listInherited.add(new URL(new URL("ftp://www.test.com/file?param1=1&param2=2"), "http://www.spec.com/"));

            System.out.println("\n--If spec has no scheme, the scheme component is inherited from the context URL.");
            for (int i = 0; i < listInherited.size(); i++) {
                System.out.println(i + ": " + listInherited.get(i));
            }
        } catch(Exception e){System.out.println(e);}

        //--The reference is parsed into the scheme, authority, path, query and fragment parts.
        try {
            URL baseURL3 = new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref");

            List<URL> listEmptyPath = new ArrayList<URL>();
            listEmptyPath.add(new URL(baseURL3, "#ref"));
            listEmptyPath.add(new URL(baseURL3, ""));
            listEmptyPath.add(new URL(baseURL3, "?query=testing&spec=2#specRef"));

            System.out.println("\n--The reference is parsed into the scheme, authority, path, query and fragment parts.");
            for (int i = 0; i < listEmptyPath.size(); i++) {
                System.out.println(i + ": " + listEmptyPath.get(i));
            }
        } catch(Exception e){System.out.println(e);}

        //--If the scheme component is defined in the given spec and does not match the scheme of the context, then the new URL is created as an absolute URL based on the spec alone. Otherwise the scheme component is inherited from the context URL.
        try {
            URL baseURL4 = new URL("ftp://www.test.com/file?param1=1&param2=2");

            List<URL> listDefinedComponent = new ArrayList<URL>();
            listDefinedComponent.add(new URL(baseURL4, "http://www.spec.com/"));
            listDefinedComponent.add(new URL(baseURL4, "ftp://www.spec.com/"));

            listDefinedComponent.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "?query=testing&spec=2#specRef"));
            System.out.println("\n--If the scheme component is defined in the given spec and does not match the scheme of the context, then the new URL is created as an absolute URL based on the spec alone. Otherwise the scheme component is inherited from the context URL");
            for (int i = 0; i < listDefinedComponent.size(); i++) {
                System.out.println(i + ": " + listDefinedComponent.get(i));
            }
        } catch(Exception e){System.out.println(e);}

        //--If the authority component is present in the spec then the spec is treated as absolute and the spec authority and path will replace the context authority and path.
        try {
            URL baseURL5 = new URL("ftp://www.test.com/file?param1=1&param2=2");

            List<URL> listAuthorityPresent = new ArrayList<URL>();
            listAuthorityPresent.add(new URL(baseURL5, "www.geeksforgeeks.org:80/url-samefile-method-in-java-with-examples/"));
            listAuthorityPresent.add(new URL(baseURL5, "www.test.com/x/file?param1=testtest"));

            System.out.println("\n--If the authority component is present in the spec then the spec is treated as absolute and the spec authority and path will replace the context authority and path");
            for (int i = 0; i < listAuthorityPresent.size(); i++) {
                System.out.println(i + ": " + listAuthorityPresent.get(i));
            }
        } catch(Exception e){System.out.println(e);}

        //--If the authority component is absent in the spec then the authority of the new URL will be inherited from the context.
        try {
            List<URL> listAuthorityAbsent = new ArrayList<URL>();
            listAuthorityAbsent.add(new URL(new URL("ftp://www.test.com/file?param1=1&param2=2"), "/url-samefile-method-in-java-with-examples/"));
            listAuthorityAbsent.add(new URL(new URL("ftp://www.test.com"), "?param1=1&param2=2"));

            System.out.println("\n--If the authority component is absent in the spec then the authority of the new URL will be inherited from the context");
            for (int i = 0; i < listAuthorityAbsent.size(); i++) {
                System.out.println(i + ": " + listAuthorityAbsent.get(i));
            }
        } catch(Exception e){System.out.println(e);}

        //--If the spec's path component begins with a slash character "/" then the path is treated as absolute and the spec path replaces the context path.
        try {
            List<URL> listSpecPathSlash = new ArrayList<URL>();
            listSpecPathSlash.add(new URL(new URL("ftp://www.test.com/"), "/filespec/spec?specQ#specref"));
            listSpecPathSlash.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "/filespec/spec#specref"));

            System.out.println("\n--TODO:If the spec's path component begins with a slash character \"/\" then the path is treated as absolute and the spec path replaces the context path");
            for (int i = 0; i < listSpecPathSlash.size(); i++) {
                System.out.println(i + ": " + listSpecPathSlash.get(i));
            }
        } catch(Exception e){System.out.println(e);}

        //--Otherwise, the path is treated as a relative path and is appended to the context path.
        try {
            URL baseURL7 = new URL("ftp://www.test.com/filecontext/con");

            List<URL> listOtherPath = new ArrayList<URL>();
            listOtherPath.add(new URL(new URL("http://www.test.com/huiui/"), "?param1=1&param2=2"));
            listOtherPath.add(new URL(baseURL7, "?param1=1&param2=2"));
            listOtherPath.add(new URL(new URL("http://www.test.com/filecontext/context?param1=1&param2=2#ref"), "moimo/kjoil=moi"));
            listOtherPath.add(new URL(baseURL7, "param1=1&param2=2"));
            listOtherPath.add(new URL(baseURL7, "#specRef"));

            System.out.println("\n--Otherwise, the path is treated as a relative path and is appended to the context path");
            for (int i = 0; i < listOtherPath.size(); i++) {
                System.out.println(i + ": " + listOtherPath.get(i));
            }
        } catch(Exception e){System.out.println(e);}


        try {
            // create a URL
            URL urlUser = new URL(
                    "https://Arnab_Kundu:pwwd1234@www.geeksforgeeks.org");

            // get the  UserInfo
            String _UserInfo = urlUser.getUserInfo();
            String _Host = urlUser.getHost();
            System.out.println("\nUserInfo:\n");
            // display the URL
            System.out.println("URL = " + urlUser);

            // display the  UserInfo
            System.out.println(" UserInfo= "
                    + _UserInfo);

            System.out.println(" Host= "
                    + _Host);
        }
        // if any error occurs
        catch (Exception e) {

            // display the error
            System.out.println(e);
        }
    }
}