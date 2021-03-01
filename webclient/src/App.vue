<template>
  <div id="app">
    <md-toolbar class="md-dense md-primary">
      <h3 class="md-title" style="flex: 1">Humidity Controller</h3>
      <md-button class="md-icon-button">
        <md-icon>power_settings_new</md-icon>
      </md-button>
      <md-button class="md-icon-button" @click="showSettings=true">
        <md-icon>settings</md-icon>
      </md-button>
    </md-toolbar>
    <div class="md-layout md-alignment-center-center">
      <div class="md-layout-item md-size-50 md-medium-size-100">
        <Main/>
      </div>
    </div>

    <md-drawer style="max-width: 100%;" :md-active.sync="showSettings">
      <Settings v-model="showSettings"/>
    </md-drawer>
  </div>
</template>

<script>
import Main from './components/Main.vue'
import Settings from './components/Settings.vue'

export default {
  name: 'App',
  components: {
    Main,
    Settings
  },
  data(){
    return {
      showSettings: false
    }
  }
}
</script>

<style lang="scss">
  @import "./scss/theme.scss";
  @import "./scss/App.scss";
</style>
