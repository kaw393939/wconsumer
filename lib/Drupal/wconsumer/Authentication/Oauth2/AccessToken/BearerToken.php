<?php
  namespace Drupal\wconsumer\Authentication\Oauth2\AccessToken;



  class BearerToken implements TokenInterface
  {
    private $accessToken;



    public function __construct($accessToken)
    {
      if (empty($accessToken))
      {
        throw new \InvalidArgumentException("Cannot create a bearer token from an empty access token");
      }

      $this->accessToken = $accessToken;
    }

    public function getToken() {
      return $this->accessToken;
    }

    public function buildAuthorizationHeader()
    {
      return "Bearer {$this->accessToken}";
    }
  }
?>