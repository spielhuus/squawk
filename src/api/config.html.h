#ifndef CONFIG_HTML_H
#define CONFIG_HTML_H

static const char* HTML_CONFIG = R"html(<!DOCTYPE html>
<html lang="en">
<head>
 <meta charset="utf-8">
 <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
 <meta name="description" content="">
 <meta name="author" content="">
 <link rel="icon" href="../../favicon.ico">

 <title>Squawk Server: Configuration</title>

 <!-- Bootstrap core CSS -->
 <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">
 <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap-theme.min.css" integrity="sha384-rHyoN1iRsVXV4nD0JutlnGaslCJuC7uwjduW9SVrLvRYooPp2bWYgmgJQIXwl/Sp" crossorigin="anonymous">

 <!-- Custom styles for this template -->
 <link href="main.css" rel="stylesheet">
</head>

<body>
 <nav class="navbar navbar-toggleable-md navbar-inverse bg-inverse fixed-top">
   <button class="navbar-toggler navbar-toggler-right" type="button" data-toggle="collapse" data-target="#navbarsExampleDefault" aria-controls="navbarsExampleDefault" aria-expanded="false" aria-label="Toggle navigation">
     <span class="navbar-toggler-icon"></span>
   </button>
   <a class="navbar-brand" href="#">Navbar</a>

   <div class="collapse navbar-collapse" id="navbarsExampleDefault">
     <ul class="navbar-nav mr-auto">
       <li class="nav-item active">
         <a class="nav-link" href="#">Home <span class="sr-only">(current)</span></a>
       </li>
       <li class="nav-item">
         <a class="nav-link" href="#">Link</a>
       </li>
       <li class="nav-item">
         <a class="nav-link disabled" href="#">Disabled</a>
       </li>
       <li class="nav-item dropdown">
         <a class="nav-link dropdown-toggle" href="http://example.com" id="dropdown01" data-toggle="dropdown" aria-haspopup="true" aria-expanded="false">Dropdown</a>
         <div class="dropdown-menu" aria-labelledby="dropdown01">
           <a class="dropdown-item" href="#">Action</a>
           <a class="dropdown-item" href="#">Another action</a>
           <a class="dropdown-item" href="#">Something else here</a>
         </div>
       </li>
     </ul>
     <form class="form-inline my-2 my-lg-0">
       <input class="form-control mr-sm-2" type="text" placeholder="Search">
       <button class="btn btn-outline-success my-2 my-sm-0" type="submit">Search</button>
     </form>
   </div>
 </nav>

 <div class="container">

    <br/><br/><br/><br/>

     <form id="configuration" action="/config" method="post">

       <div class="form-group row">
         <label for="name" class="col-sm-2 col-form-label">Name</label>
         <div class="col-sm-10">
           <input type="text" class="form-control" id="name" name="name" placeholder="Server Display Name" value="{{ config:name }}"></input>
         </div>
       </div>

       <div class="form-group row">
         <label for="mediaFormControlSelect">Directories</label>
         <div class="col-sm-10">
           <input type="text" name="other"/>
           <button type="button" class="btn btn-default" aria-label="Left Align">
             <span class="glyphicon glyphicon-align-left" aria-hidden="true"></span>
           </button>
           <button type="button" class="btn btn-default" aria-label="Left Align">
             <span class="glyphicon glyphicon-align-left"></span>
           </button>

           <select multiple class="form-control" id="mediaFormControlSelect" name="media">
             {% for directory in media %}
               <option value="{{ directory }}">{{ directory }}</option>
             {% endfor %}
           </select>
         </div>
       </div>

      <div class="form-group row">
       <label for="listen_address" class="col-sm-2 col-form-label">UPNP IP</label>
       <div class="col-sm-10">
         <input type="text" class="form-control" id="listen_address" name="listen_address" placeholder="" value="{{ config:discovery:upnp:listen_address }}"></input>
       </div>
      </div>

       <div class="form-group row">
        <label for="inputEmail3" class="col-sm-2 col-form-label">UPNP PORT</label>
        <div class="col-sm-10">
          <input type="text" class="form-control" id="inputEmail3" placeholder="{{ config:discovery:upnp:listen_port }}">
        </div>
       </div>

       <div class="form-group row">
        <label for="inputEmail3" class="col-sm-2 col-form-label">SSDP IP</label>
        <div class="col-sm-10">
          <input type="email" class="form-control" id="inputEmail3" placeholder="{{ config:discovery:upnp:multicast_address }}">
        </div>
       </div>

        <div class="form-group row">
         <label for="inputEmail3" class="col-sm-2 col-form-label">SSDP PORT</label>
         <div class="col-sm-10">
           <input type="email" class="form-control" id="inputEmail3" placeholder="{{ config:discovery:upnp:multicast_port }}">
         </div>
        </div>

         <div class="form-group row">
          <label for="inputEmail3" class="col-sm-2 col-form-label">API IP</label>
          <div class="col-sm-10">
            <input type="text" class="form-control" id="inputEmail3" placeholder="{{ config:api:listen_address }}">
          </div>
         </div>

          <div class="form-group row">
           <label for="inputEmail3" class="col-sm-2 col-form-label">API PORT</label>
           <div class="col-sm-10">
             <input type="text" class="form-control" id="inputEmail3" placeholder="{{ config:api:listen_port }}">
           </div>
          </div>

           <div class="form-group row">
            <label for="inputEmail3" class="col-sm-2 col-form-label">UUID</label>
            <div class="col-sm-10">
              <input type="text" class="form-control" id="inputEmail3" placeholder="{{ config:discovery:uuid }}">
            </div>
           </div>

           <div class="form-group row">
            <label for="inputMusicbrainz" class="col-sm-2 col-form-label">musicbrainz uri</label>
            <div class="col-sm-10">
              <input type="text" class="form-control" id="inputMusicbrainz" placeholder="{{ config:musicbrainz }}">
            </div>
           </div>

       <div class="form-group row">
         <div class="offset-sm-2 col-sm-10">
           <button type="submit" class="btn btn-primary">Sign in</button>
         </div>
       </div>
     </form>

 </div><!-- /.container -->


 <!-- Bootstrap core JavaScript
 ================================================== -->
 <!-- Placed at the end of the document so the pages load faster -->
 <script src="https://code.jquery.com/jquery-3.1.1.slim.min.js" integrity="sha384-A7FZj7v+d/sdmMqp/nOQwliLvUsJfDHW+k9Omg/a/EheAdgtzNs3hpfag6Ed950n" crossorigin="anonymous"></script>
 <!-- script>window.jQuery || document.write('<script src="../../assets/js/vendor/jquery.min.js"><\/script>')</script -->
 <script src="https://cdnjs.cloudflare.com/ajax/libs/tether/1.4.0/js/tether.min.js" integrity="sha384-DztdAPBWPRXSA/3eYEEUWrWCy7G5KFbe8fFjk5JAIxUYHKkDx6Qin1DkWx51bBrb" crossorigin="anonymous"></script>
 <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script>

 <!-- IE10 viewport hack for Surface/desktop Windows 8 bug -->
 <!-- script src="../../assets/js/ie10-viewport-bug-workaround.js"></script -->
</body>
</html>
)html";

#endif // CONFIG_HTML_H
