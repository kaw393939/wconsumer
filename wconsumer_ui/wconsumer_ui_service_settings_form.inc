<?php
use Drupal\wconsumer\Wconsumer;
use Drupal\wconsumer\Service\Service;
use Drupal\wconsumer\Authentication\Oauth\Oauth;
use Drupal\wconsumer\Authentication\Oauth2\Oauth2;
use Drupal\wconsumer\Authentication\Credentials;



function wconsumer_ui_service_settings_form($form, &$form_state, $serviceName = NULL) {
  if (!isset($serviceName)) {
    $form['dummy'] = array(
      '#markup' => t('Please click on a tab above to configure an individual service.'),
    );

    return $form;
  }

  $usesOauth = function(Service $service) {
    return
      $service->authentication instanceof Oauth ||
      $service->authentication instanceof Oauth2;
  };

  $service = Wconsumer::instance()->services->{$serviceName};

  wconsumer_ui_service_settings_form_https_support_warning($service);

  $meta = $service->getMeta();
  $tokenLabel = $meta->consumerKeyLabel;
  $secretLabel = $meta->consumerSecretLabel;

  $tokenPredefined =
      $service->authentication instanceof \Drupal\wconsumer\Authentication\QueryString\QueryString &&
      isset($service->authentication->queryKey);

  $credentials = $service->getServiceCredentials();
  $token =  (isset($credentials) ? $credentials->token  : NULL);
  $secret = (isset($credentials) ? $credentials->secret : NULL);

  $form['creds'] = array(
    '#type' => 'fieldset',
    '#title' => t($serviceName.' Application Credentials'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
   );

  if ($usesOauth($service)) {
    $form['creds']['note'] = wconsumer_ui_service_settings_form_note($service);
  }

  $form['creds']['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable service'),
    '#default_value' => $service->isEnabled(),
  );

  if (!$tokenPredefined) {
    $form['creds']['token'] = array(
      '#type' => 'textfield',
      '#title' => t($tokenLabel),
      '#required' => TRUE,
      '#default_value' => $token,
    );
  }
  else {
    $form['creds']['token'] = array(
      '#type' => 'hidden',
      '#required' => TRUE,
      '#default_value' => (isset($token) ? $token : 'dummy'),
    );
  }

  $form['creds']['secret'] = array(
    '#type' => 'textfield',
    '#title' => t($secretLabel),
    '#required' => TRUE,
    '#default_value' => $secret,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Configuration')
  );

  $form['reset_auth'] = array(
    '#type' => 'submit',
    '#value' => t('Reset all users auth tokens'),
    '#submit' => array('wconsumer_ui_reset_auth'),
    '#prefix' => '<br><br>',
    '#attributes' => array('onclick' => 'return confirm("That would make all users have authorize service again. Are you sure?");')
  );

  $form['service-name'] = array(
    '#type' => 'hidden',
    '#value' => $serviceName,
  );

  return $form;
}

function wconsumer_ui_service_settings_form_note(Service $service) {
  $sentences = array();
  $fields = array();
  {
    // register an application
    $registerAppUrl = $service->getMeta()->registerAppUrl;
    if (isset($registerAppUrl)) {
      $sentences[] =
        'In order to use this service you have to
         <a href="@registerUrl" target="_blank">register an application</a>';

      $fields['@registerUrl'] = $registerAppUrl;
    }
    else {
      $sentences[] = 'In order to use this service you have to register an application';
    }

    // use callback uri
    {
      $sentences[] =
        'Use "!callbackUrl" (without quotes) as the Callback URL when registering your app (this field might
         also be called Redirect URL or Success Callback URL)';

      // It would be better to specify base website url as the callback. However, some of the services like
      // Google require callback url to be exactly matching to the one specified in app settings. So we'd better
      // use full callback url for all services rather then varying it for some of them.
      $callbackUrl = $service->getCallbackUrl();

      $callbackUrl = check_plain($callbackUrl);
      $callbackUrl = preg_replace('|^(https://)|', '<b>\1</b>', $callbackUrl);

      $fields['!callbackUrl'] = $callbackUrl;
    }

    // callback uri change note
    if ($service->authentication instanceof Oauth) {
      $sentences[] = 'Actually, you can change it to anything you want if you need it for some reason';
    }
    else {
      $sentences[] =
        'Please do not change it unless you know what you do because most likely it will
        make the service inaccessible';
    }

    // fill in credentials
    $sentences[] = 'Once you created your app come back here and fill in app credentials you\'ve got';
  }

  foreach ($sentences as &$sentence) {
    $sentence = t($sentence, $fields).'. ';
  }

  return array(
    '#markup' => join('', $sentences).'<br /><br />',
  );
}

function wconsumer_ui_service_settings_form_https_support_warning(Service $service) {
  global $base_url;

  $service_requires_https = (strpos($service->getCallbackUrl(), 'https://') === 0);
  $drupal_uses_https = (strpos($base_url, 'https://') === 0);

  if (!$drupal_uses_https && $service_requires_https) {
    drupal_set_message(t('
      HTTPS support required to make @service work. However it looks like this Drupal installation
      does not use HTTPS. Please make sure your website is properly configured for HTTPS support.
    ', array(
      '@service' => $service->getMeta()->niceName,
    )), 'warning');
  }
}

function wconsumer_ui_service_settings_form_submit($form, &$form_state) {
  $values = $form_state['input'];

  $serviceName = @$values['service-name'];
  if (!isset($serviceName)) {
    drupal_exit();
  }

  $service = Wconsumer::instance()->services->{$serviceName};

  $service->setEnabled((bool)$values['enabled']);

  try {
    $newCredentials = Credentials::fromArray($values);
  } catch (\Drupal\wconsumer\Exception $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    return;
  }

  if (!$service->validateServiceCredentials($newCredentials)) {
    drupal_set_message(t('The service reports that the new credentials are not valid. Please double-check them.'), 'error');
    return;
  }

  $previousCredentianls = $service->getServiceCredentials();

  $service->setServiceCredentials($newCredentials);
  drupal_set_message(t('Service configuration saved.'));

  if (!Credentials::equal($previousCredentianls, $newCredentials)) {
    drupal_set_message(t(
      'You have just changed service app credentials. '.
      'Please note that users authorized by this moment will continue using the previous app. '.
      'Although currently existing auth tokens will continue working you may want to reset them and force users to reauthorize with the new app. '.
      'To do this click "Reset all users auth tokens" button below.'
    ), 'warning');
  }
}

function wconsumer_ui_reset_auth($form, $form_state) {
  $service = Wconsumer::instance()->services->{$form_state['input']['service-name']};

  $uids =
    db_select('users', 'u')
      ->condition('u.uid', 0, '<>')
      ->fields('u', array('uid'))
    ->execute()
    ->fetchCol();

  foreach ($uids as $uid) {
    $service->setCredentials(NULL, $uid);
  }

  drupal_set_message(t('All auth tokens has been reset.'));
}

/**
 * Callback for the Service
 */
function _wconsumer_ui_callback($serviceName = NULL) {
  global $user;

  $service = Wconsumer::instance()->services->{$serviceName};

  $userOldCredentials = $service->getCredentials($user->uid);
    $service->authentication->onCallback($user, array($_REQUEST));
  $userNewCredentials = $service->getCredentials($user->uid);

  if (user_is_logged_in() && !Credentials::equal($userOldCredentials, $userNewCredentials)) {
    drupal_set_message(t(
      "Your @service account is now linked with your local account.",
      array('@service' => $service->getMeta()->niceName))
    );
  }

  _wconsumer_ui_goto();
}